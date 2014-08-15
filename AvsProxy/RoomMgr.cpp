#include "RoomMgr.h"

extern SafeUdpSocket g_safe_udp_sock;

void RoomMgr::event_on_tcp_accept(evutil_socket_t fd, short event, void *arg)
{
	RoomMgr *mgr = static_cast<RoomMgr *>(arg);
	mgr->EventOnTcpAccept(fd, event);
}

void RoomMgr::event_on_tcp_read(evutil_socket_t fd, short event, void *arg)
{
	RoomMgr *mgr = static_cast<RoomMgr *>(arg);
	mgr->EventOnTcpRead(fd, event);
}

void RoomMgr::event_on_udp_read(evutil_socket_t fd, short event, void *arg)
{
	RoomMgr *mgr = static_cast<RoomMgr *>(arg);
	mgr->EventOnUdpRead(fd, event);
}

RoomMgr::RoomMgr(const pj_str_t &local_ip,
				 pj_uint16_t local_tcp_port,
				 pj_uint16_t local_udp_port,
				 pj_uint8_t  thread_pool_size)
	: Noncopyable()
	, local_tcp_sock_(-1)
	, local_udp_sock_(-1)
	, local_ip_(pj_str(local_ip.ptr))
	, local_tcp_port_(local_tcp_port)
	, local_udp_port_(local_udp_port)
	, caching_pool_()
	, thread_pool_size_(thread_pool_size)
	, event_thread_()
	, tcp_ev_(NULL)
	, udp_ev_(NULL)
	, evbase_(NULL)
	, active_(PJ_FALSE)
	, terminations_()
	, rooms_()
	, rtp_in_session_()
	, rtp_out_session_()
	, sync_thread_pool_(1)
	, async_thread_pool_(thread_pool_size)
{
}

pj_status_t RoomMgr::Prepare(const pj_str_t &log_file_name)
{
	pj_status_t status;
	pj_uint8_t retrys = 50;
	do
	{
		status = pj_open_tcp_serverport(&local_ip_, local_tcp_port_, local_tcp_sock_);
	} while(status != PJ_SUCCESS && ((++ local_tcp_port_), (-- retrys > 0)));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	retrys = 50;
	do
	{
		status = pj_open_udp_transport(&local_ip_, local_udp_port_, local_udp_sock_);
	} while(status != PJ_SUCCESS && ((++ local_udp_port_), (-- retrys > 0)));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	status = g_safe_udp_sock.Open();
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	pj_caching_pool_init(&caching_pool_, &pj_pool_factory_default_policy, 0);

	pool_ = pj_pool_create(&caching_pool_.factory, "AvsProxyPool", 1000, 1000, NULL);

	status = log_open(pool_, log_file_name);

	status = pjmedia_rtp_session_init(&rtp_in_session_, RTP_EXPAND_PAYLOAD_TYPE, pj_rand());
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	status = pjmedia_rtp_session_init(&rtp_out_session_, 0, 0);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	evbase_ = event_base_new();
	RETURN_VAL_IF_FAIL( evbase_ != nullptr, -1 );

	tcp_ev_ = event_new(evbase_, local_tcp_sock_, EV_READ | EV_PERSIST, event_on_tcp_accept, this);
	RETURN_VAL_IF_FAIL( tcp_ev_ != nullptr, -1 );

	udp_ev_ = event_new(evbase_, local_udp_sock_, EV_READ | EV_PERSIST, event_on_udp_read, this);
	RETURN_VAL_IF_FAIL( udp_ev_ != nullptr, -1 );

	int ret;
	ret = event_add(tcp_ev_, NULL);
	RETURN_VAL_IF_FAIL( ret == 0, -1 );

	ret = event_add(udp_ev_, NULL);
	RETURN_VAL_IF_FAIL( ret == 0, -1 );

	return status;
}

pj_status_t RoomMgr::Launch()
{
	active_ = PJ_TRUE;
	event_thread_ = thread(std::bind(&RoomMgr::EventThread, this));
	sync_thread_pool_.Start();
	async_thread_pool_.Start();

	return PJ_SUCCESS;
}

void RoomMgr::Destroy()
{
	active_ = PJ_FALSE;
	g_safe_udp_sock.Close();
	sync_thread_pool_.Stop();
	async_thread_pool_.Stop();
	event_base_loopexit(evbase_, NULL);

	pj_sock_close(local_tcp_sock_);
	pj_sock_close(local_udp_sock_);
}

room_map_t::mapped_type RoomMgr::GetRoom(room_map_t::key_type room_id)
{
	room_map_t::mapped_type room = nullptr;
	room_map_t::iterator proom = rooms_.find(room_id);
	if ( proom != rooms_.end() )
	{
		room = proom->second;
	}

	return room;
}

void RoomMgr::TcpParamScene(Termination *termination,
							const pj_uint8_t *storage,
							pj_uint16_t storage_len)
{
	TcpParameter *param = NULL;
	TcpScene     *scene = NULL;
	Room         *room = NULL;
	pj_uint16_t type = (pj_uint16_t)ntohs(*(pj_uint16_t *)(storage + sizeof(param->length_)));

	switch(type)
	{
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN:
		{
			param = new LoginParameter(storage, storage_len);
			scene = new LoginScene();
			sync_thread_pool_.Schedule([=]()
			{
				static_cast<LoginScene *>(scene)->Maintain(param, termination, rooms_); delete scene; delete param;
			});
			return;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGOUT:
		{
			param = new LogoutParameter(storage, storage_len);
			scene = new LogoutScene();
		    break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER:
		{
			param = new LinkRoomUserParameter(storage, storage_len);
			scene = new LinkRoomUserScene();
			room  = GetRoom(reinterpret_cast<LinkRoomUserParameter *>(param)->room_id_);
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_UNLINK_ROOM_USER:
		{
			param = new UnlinkRoomUserParameter(storage, storage_len);
			scene = new UnlinkRoomUserScene();
			room  = GetRoom(reinterpret_cast<UnlinkRoomUserParameter *>(param)->room_id_);
			break;
		}
	}

	sync_thread_pool_.Schedule([=]()
	{
		scene->Maintain(param, termination, room); delete scene; delete param;
	});
}

void RoomMgr::UdpParamScene(const pj_uint8_t *storage,
							pj_uint16_t storage_len)
{
	UdpParameter *param = NULL;
	UdpScene     *scene = NULL;
	Room         *room = NULL;
	pj_uint16_t type = ntohs(*(pj_uint16_t *)(storage));

	switch(type)
	{
		case REQUEST_FROM_AVS_TO_AVSPROXY_USERS_INFO:
		{
			param = new UsersInfoParameter(storage, storage_len);
			scene = new UsersInfoScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_MOD_USER_MEDIA:
		{
			param = new ModUserMediaParameter(storage, storage_len);
			scene = new ModUserMediaScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_ADD_USER:
		{
			param = new AddUserParameter(storage, storage_len);
			scene = new AddUserScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_DEL_USER:
		{
			param = new DelUserParameter(storage, storage_len);
			scene = new DelUserScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM:
		{
			param = new RTPParameter(storage, storage_len);
			scene = new RTPScene();
			break;
		}
	}

	room = GetRoom(param->room_id_);

	if ( type == REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM )
	{
		async_thread_pool_.Schedule([=]()
		{
			scene->Maintain(param, room); delete scene; delete param;
		});
	}
	else
	{
		sync_thread_pool_.Schedule([=]()
		{
			scene->Maintain(param, room); delete scene; delete param;
		});
	}
}

void RoomMgr::EventOnTcpAccept(evutil_socket_t fd, short event)
{
	pj_status_t status;
	pj_sock_t term_sock = PJ_INVALID_SOCKET;
	pj_sockaddr_in term_addr;
	int addr_len = sizeof(term_addr);

	status = pj_sock_accept(local_tcp_sock_, &term_sock, &term_addr, &addr_len);
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	u_long val = 1;
#if defined(PJ_WIN32) && PJ_WIN32!=0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0
    if (ioctlsocket(term_sock, FIONBIO, &val)) {
#else
    if (ioctl(new_sock, FIONBIO, &val)) {
#endif
        pj_sock_close(term_sock);
		return;
    }

	pj_in_addr addr = pj_sockaddr_in_get_addr(&term_addr);
	char *addr_ip = pj_inet_ntoa(addr);
	pj_str_t pj_addr_ip = pj_str(addr_ip);

	status = AddTermination(pj_addr_ip, term_sock);
	RETURN_IF_FAIL( status == PJ_SUCCESS );
}

pj_status_t RoomMgr::AddTermination(const pj_str_t &ip, pj_sock_t fd)
{
	termination_map_t::iterator ptermination = terminations_.find(fd);
	RETURN_VAL_IF_FAIL( ptermination == terminations_.end(), PJ_EEXISTS );

	termination_map_t::mapped_type termination = new Termination(ip, fd);
	pj_assert( termination != nullptr );

	terminations_.insert(termination_map_t::value_type(fd, termination));

	struct event *ev = event_new(evbase_, fd, EV_READ | EV_PERSIST, event_on_tcp_read, this);
	termination->tcp_ev_ = ev;

	int evret;
	evret = event_add(ev, NULL);

	return PJ_SUCCESS;
}

pj_status_t RoomMgr::DelTermination(pj_sock_t fd)
{
	termination_map_t::iterator ptermination = terminations_.find(fd);
	RETURN_VAL_IF_FAIL( ptermination != terminations_.end(), PJ_ENOTFOUND );

	termination_map_t::mapped_type termination = ptermination->second;
	terminations_.erase(ptermination); // First free the memory of iterator, termination is safe.

	RETURN_VAL_IF_FAIL( termination != nullptr, PJ_SUCCESS );

	pj_sock_close( termination->tcp_socket_ );
	event_del( termination->tcp_ev_ );
	delete termination;
	termination = nullptr;

	return PJ_SUCCESS;
}

void RoomMgr::EventOnTcpRead(evutil_socket_t fd, short event)
{
	pj_sock_t client_tcp_sock = fd;
	pj_status_t status;
	termination_map_t::iterator ptermination = terminations_.find(client_tcp_sock);
	RETURN_IF_FAIL( ptermination != terminations_.end() );
	termination_map_t::mapped_type termination = ptermination->second;
	RETURN_WITH_STATEMENT_IF_FAIL( termination != nullptr, terminations_.erase(ptermination) );

	RETURN_IF_FAIL( event & EV_READ );

	pj_ssize_t recvlen = MAX_STORAGE_SIZE - termination->tcp_storage_offset_;
	status = pj_sock_recv(client_tcp_sock,
		(char *)(termination->tcp_storage_ + termination->tcp_storage_offset_),
		&recvlen,
		0 );
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	if ( recvlen > 0 )
	{
		termination->tcp_storage_offset_ += (pj_uint16_t)recvlen;
		pj_uint16_t packet_len = ntohs(*(pj_uint16_t *)termination->tcp_storage_);
		pj_uint16_t total_len  = packet_len + sizeof(packet_len);

		if ( total_len > MAX_STORAGE_SIZE )
		{
			termination->tcp_storage_offset_ = 0;
		}
		else if ( total_len > termination->tcp_storage_offset_ )
		{
			return;
		}
		else if ( total_len <= termination->tcp_storage_offset_ )
		{
			termination->tcp_storage_offset_ -= total_len;

			TcpParamScene(termination, termination->tcp_storage_, total_len);

			if ( termination->tcp_storage_offset_ > 0 )
			{
				memcpy(termination->tcp_storage_,
					termination->tcp_storage_ + total_len,
					termination->tcp_storage_offset_);
			}
		}
	}
	else if ( recvlen == 0 )
	{
		DelTermination(client_tcp_sock);
	}
	else /* if ( recvlen < 0 ) */
	{
		DelTermination(client_tcp_sock);
	}
}

void RoomMgr::EventOnUdpRead(evutil_socket_t fd, short event)
{
	pj_uint8_t datagram[MAX_UDP_DATA_SIZE];
	pj_ssize_t datalen = MAX_UDP_DATA_SIZE;
	pj_sock_t local_udp_sock = fd;
	const pjmedia_rtp_hdr *rtp_hdr;

	const pj_uint8_t *payload;
    unsigned payload_len;

	pj_sockaddr_in addr;
	int addrlen = sizeof(addr);

	pj_status_t status;
	status = pj_sock_recvfrom(local_udp_sock, datagram, &datalen, 0, &addr, &addrlen);
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	UdpParameter *param = NULL;
	UdpScene *scene = NULL;
	Room *room = NULL;

	if ( datalen >= sizeof(*rtp_hdr) )
	{
		status = pjmedia_rtp_decode_rtp(&rtp_in_session_, 
				    datagram, (int)datalen, 
				    &rtp_hdr, (const void **)&payload, &payload_len);
		RETURN_IF_FAIL( status == PJ_SUCCESS );

		UdpParamScene(payload, payload_len);
		
		 // Only for media stream.
		if ( param->avs_request_type_ == REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM )
		{
			async_thread_pool_.Schedule([=]()
			{
				scene->Maintain(param, room);
				delete scene;
				delete param;
			});
		}
		else
		{
			sync_thread_pool_.Schedule([=]()
			{
				scene->Maintain(param, room);
				delete scene;
				delete param;
			});
		}

		pjmedia_rtp_session_update(&rtp_in_session_, rtp_hdr, NULL);
	}
}

void RoomMgr::EventThread()
{
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;
	
	if ( !pj_thread_is_registered() )
	{
		if ( pj_thread_register(NULL,rtpdesc,&thread) == PJ_SUCCESS )
		{
			while ( active_ )
			{
				event_base_loop(evbase_, EVLOOP_ONCE);
			}
		}
	}
}

void RoomMgr::Login(pj_str_t *ip, pj_uint16_t port, pj_int32_t room_id)
{
	pj_uint8_t packet[MAX_UDP_DATA_SIZE];

	memset(packet, 0, MAX_UDP_DATA_SIZE);

	const pjmedia_rtp_hdr *hdr;
	const void *p_hdr;
	int hdrlen;
	pj_ssize_t size;

	pj_status_t status;
	status = pjmedia_rtp_encode_rtp (&rtp_out_session_, RTP_EXPAND_PAYLOAD_TYPE,
		0, sizeof(request_to_avs_login_t), 160, &p_hdr, &hdrlen);
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	request_to_avs_login_t req_login;
	req_login.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LOGIN;
	req_login.proxy_id = 100;
	req_login.room_id = room_id;

	// req_login.Serialize();

	hdr = (const pjmedia_rtp_hdr*) p_hdr;

	/* Copy RTP header to packet */
	pj_memcpy(packet, hdr, hdrlen);

	/* Copy RTP payload to packet */
	pj_memcpy(packet + hdrlen, &req_login, sizeof(req_login));

	/* Send RTP packet */
	size = hdrlen + sizeof(request_to_avs_login_t);

	{
		room_map_t::mapped_type room = new Room();
		pj_assert( room != nullptr );
		rooms_[room_id] = room;
	}

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, ip, port);
	status = pj_sock_sendto(local_udp_sock_, packet, &size, 0, &addr, sizeof(addr));
}

void RoomMgr::Logout(pj_str_t *ip, pj_uint16_t port, pj_int32_t room_id)
{
	pj_uint8_t packet[MAX_UDP_DATA_SIZE];

	memset(packet, 0, MAX_UDP_DATA_SIZE);

	const pjmedia_rtp_hdr *hdr;
	const void *p_hdr;
	int hdrlen;
	pj_ssize_t size;

	pj_status_t status;
	status = pjmedia_rtp_encode_rtp (&rtp_out_session_, RTP_EXPAND_PAYLOAD_TYPE,
		0, sizeof(request_to_avs_logout_t), 160, &p_hdr, &hdrlen);
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	request_to_avs_logout_t req_logout;
	req_logout.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LOGOUT;
	req_logout.proxy_id = 100;
	req_logout.room_id = room_id;

	// req_logout.Serialize();

	hdr = (const pjmedia_rtp_hdr*) p_hdr;

	/* Copy RTP header to packet */
	pj_memcpy(packet, hdr, hdrlen);

	/* Copy RTP payload to packet */
	pj_memcpy(packet + hdrlen, &req_logout, sizeof(req_logout));

	/* Send RTP packet */
	size = hdrlen + sizeof(request_to_avs_logout_t);

	{
		room_map_t::iterator proom = rooms_.begin();
		room_map_t::mapped_type room = proom->second;
		if ( proom != rooms_.end() )
		{
			rooms_.erase(proom);
			if ( room != nullptr )
			{
				delete room;
				room = nullptr;
			}
		}
	}

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, ip, port);
	status = pj_sock_sendto(local_udp_sock_, packet, &size, 0, &addr, sizeof(addr));
}
