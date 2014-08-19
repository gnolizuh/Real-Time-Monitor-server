#include "RoomMgr.h"

extern SafeUdpSocket g_safe_client_sock;

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

	retrys = 50;
	do
	{
		status = g_safe_client_sock.Open();
	} while(status != PJ_SUCCESS && (-- retrys > 0));
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
	g_safe_client_sock.Close();
	sync_thread_pool_.Stop();
	async_thread_pool_.Stop();
	event_base_loopexit(evbase_, NULL);

	pj_sock_close(local_tcp_sock_);
	pj_sock_close(local_udp_sock_);
}

room_map_t::mapped_type RoomMgr::GetRoom(room_map_t::key_type room_id)
{
	lock_guard<mutex> lock(rooms_lock_);
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
	Room         *room  = NULL;
	pj_uint16_t type = (pj_uint16_t)ntohs(*(pj_uint16_t *)(storage + sizeof(param->length_)));

	switch(type)
	{
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN:
		{
			param = new LoginParameter(storage, storage_len);
			scene = new LoginScene();
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
			room  = GetRoom(static_cast<LinkRoomUserParameter *>(param)->room_id_);
			sync_thread_pool_.Schedule([=]()
			{
				scene_opt_t scene_opt;
				pj_buffer_t buffer;
				scene_opt = static_cast<LinkRoomUserScene *>(scene)->Maintain(param, room, termination, buffer);
				delete scene;
				delete param;
				this->AfterMaintain(scene_opt, buffer);
			});
			return;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_UNLINK_ROOM_USER:
		{
			param = new UnlinkRoomUserParameter(storage, storage_len);
			scene = new UnlinkRoomUserScene();
			room  = GetRoom(static_cast<UnlinkRoomUserParameter *>(param)->room_id_);
			sync_thread_pool_.Schedule([=]()
			{
				scene_opt_t scene_opt;
				pj_buffer_t buffer;
				scene_opt = static_cast<UnlinkRoomUserScene *>(scene)->Maintain(param, room, termination, buffer);
				delete scene;
				delete param;
				this->AfterMaintain(scene_opt, buffer);
			});
			return;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_KEEP_ALIVE:
		{
			param = new KeepAliveParameter(storage, storage_len);
			scene = new KeepAliveScene();
			break;
		}
	}

	sync_thread_pool_.Schedule([=]()
	{
		scene_opt_t scene_opt;
		pj_buffer_t buffer;
		scene_opt = scene->Maintain(param, termination, buffer); delete scene; delete param;
		this->AfterMaintain(scene_opt, buffer);
	});
}

void RoomMgr::UdpParamScene(const pj_uint8_t *storage,
							pj_uint16_t storage_len)
{
	UdpParameter *param = NULL;
	UdpScene     *scene = NULL;
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
			/* Proxy在收到Avs发来的ModUser之后, 需要更新所有在线的Client */
			param = new ModUserMediaParameter(storage, storage_len);
			scene = new ModUserMediaScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_ADD_USER:
		{
			/* Proxy在收到Avs发来的AddUser之后, 需要更新所有在线的Client */
			param = new AddUserParameter(storage, storage_len);
			scene = new AddUserScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_DEL_USER:
		{
			/* Proxy在收到Avs发来的DelUser之后, 需要更新所有在线的Client */
			param = new DelUserParameter(storage, storage_len);
			scene = new DelUserScene();
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM:
		{
			/* Proxy在收到Avs发来的RTP包之后, 需要向所有在线的Client转发 */
			param = new RTPParameter(storage, storage_len);
			scene = new RTPScene();
			break;
		}
		case RESPONSE_FROM_AVS_TO_AVSPROXY_LOGIN:
		{
			param = new ResLoginParameter(storage, storage_len);
			scene = new ResLoginScene();
			break;
		}
		case RESPONSE_FROM_AVS_TO_AVSPROXY_KEEP_ALIVE:
		{
			param = new ResKeepAliveParameter(storage, storage_len);
			scene = new ResKeepAliveScene();
			break;
		}
	}

	Room *room = GetRoom(param->room_id_);

	if ( type == REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM )
	{
		async_thread_pool_.Schedule([=]()
		{
			scene_opt_t scene_opt;
			pj_buffer_t buffer;
			scene_opt = scene->Maintain(param, room, buffer); delete scene; delete param;
			this->AfterMaintain(scene_opt, buffer);
		});
	}
	else
	{
		sync_thread_pool_.Schedule([=]()
		{
			scene_opt_t scene_opt;
			pj_buffer_t buffer;
			scene_opt = scene->Maintain(param, room, buffer); delete scene; delete param;
			this->AfterMaintain(scene_opt, buffer);
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
	lock_guard<mutex> lock(terminations_lock_);
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
	lock_guard<mutex> lock(terminations_lock_);
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
	
	terminations_lock_.lock();
	termination_map_t::iterator ptermination = terminations_.find(client_tcp_sock);
	RETURN_IF_FAIL( ptermination != terminations_.end() );
	termination_map_t::mapped_type termination = ptermination->second;
	RETURN_WITH_STATEMENT_IF_FAIL( termination != nullptr, terminations_.erase(ptermination) );
	terminations_lock_.unlock();

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


void RoomMgr::SendTCPPacketToAllClient(pj_buffer_t &buffer)
{
	lock_guard<mutex> lock(terminations_lock_);
	termination_map_t::iterator ptermination = terminations_.begin();
	for(; ptermination != terminations_.end(); ++ ptermination)
	{
		termination_map_t::mapped_type termination = ptermination->second;
		if(termination != nullptr)
		{
			pj_ssize_t sndlen = buffer.size();
			termination->SendTCPPacket(&buffer, &sndlen);
		}
	}
}

void RoomMgr::SendRTPPacketToAllAvs(pj_buffer_t &buffer)
{
	lock_guard<mutex> lock(rooms_lock_);
	room_map_t::iterator proom = rooms_.begin();
	for(; proom != rooms_.end(); ++ proom)
	{
		room_map_t::mapped_type room = proom->second;
		if(room != nullptr)
		{
			pj_ssize_t sndlen = buffer.size();
			room->SendRTPPacket(&buffer, &sndlen);
		}
	}
}

pj_status_t RoomMgr::SendRTPPacket(const pj_str_t &ip, pj_uint16_t port, int pt, void *payload, int payload_len)
{
	RETURN_VAL_IF_FAIL(payload_len <= MAX_UDP_DATA_SIZE, PJ_EINVAL);

	pj_uint8_t packet[MAX_UDP_DATA_SIZE];
	const pjmedia_rtp_hdr *hdr;
	const void *p_hdr;
	int hdrlen;
	pj_ssize_t size;
	
	pj_status_t status;
	status = pjmedia_rtp_encode_rtp (&rtp_out_session_, pt,	0, payload_len, 0, &p_hdr, &hdrlen);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	hdr = (const pjmedia_rtp_hdr*) p_hdr;

	/* Copy RTP header to packet */
	pj_memcpy(packet, hdr, hdrlen);

	/* Copy RTP payload to packet */
	pj_memcpy(packet + hdrlen, payload, payload_len);

	/* Send RTP packet */
	size = hdrlen + payload_len;

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, &ip, port);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	return pj_sock_sendto(local_udp_sock_, packet, &size, 0, &addr, sizeof(addr));
}

void RoomMgr::AfterMaintain(scene_opt_t opt, pj_buffer_t &buffer)
{
	RETURN_IF_FAIL(opt != SCENE_OPT_NONE);

	switch(opt)
	{
		case SCENE_OPT_TCP_TO_CLIENT:
		{
			pj_uint16_t client_type = *(pj_uint16_t *)&buffer[2];
			switch(client_type)
			{
				case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_ADD_USER:
				case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_DEL_USER:
				case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_MOD_MEDIA:
				{
					SendTCPPacketToAllClient(buffer);
				}
			}
		}
		case SCENE_OPT_RTP_TO_AVS:
		{
			pj_uint16_t avs_type = *(pj_uint16_t *)&buffer[0];
			pj_int32_t room_id = *(pj_int32_t *)&buffer[4];
			switch(avs_type)
			{
				case REQUEST_FROM_AVSPROXY_TO_AVS_LINK_USER:
				case REQUEST_FROM_AVSPROXY_TO_AVS_UNLINK_USER:
				{
					SendRTPPacketToAllAvs(buffer);
				}
			}
		}
	}
}

pj_status_t RoomMgr::Login(pj_str_t *ip, pj_uint16_t port, pj_int32_t room_id)
{
	request_to_avs_login_t req_login;
	req_login.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LOGIN;
	req_login.proxy_id = 100;
	req_login.room_id = room_id;

	// req_login.Serialize();

	rooms_lock_.lock();
	room_map_t::iterator proom = rooms_.find(room_id);
	room_map_t::mapped_type room;
	if(proom == rooms_.end())
	{
		room = new Room(*ip, port, local_udp_sock_, rtp_out_session_, rtp_out_sess_lock_);
		pj_assert( room != nullptr );
		room->Prepare();
		room->Launch();
	}
	else
	{
		room = proom->second;
	}
	rooms_[room_id] = room;
	rooms_lock_.unlock();

	pj_ssize_t sndlen = sizeof(req_login);
	return room->SendRTPPacket(&req_login, &sndlen);
}

pj_status_t RoomMgr::Logout(pj_str_t *ip, pj_uint16_t port, pj_int32_t room_id)
{
	request_to_avs_logout_t req_logout;
	req_logout.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LOGOUT;
	req_logout.proxy_id = 100;
	req_logout.room_id = room_id;

	// req_logout.Serialize();

	rooms_lock_.lock();
	pj_status_t status = PJ_SUCCESS;
	room_map_t::iterator proom = rooms_.find(room_id);
	if(proom != rooms_.end())
	{
		room_map_t::mapped_type room = proom->second;
		rooms_.erase(proom);
		if (room != nullptr)
		{
			pj_ssize_t sndlen = sizeof(req_logout);
			status = room->SendRTPPacket(&req_logout, &sndlen);
			room->Destory();
			delete room;
			room = nullptr;
		}
	}
	rooms_lock_.unlock();

	return status;
}
