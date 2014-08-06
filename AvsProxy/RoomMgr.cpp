#include "RoomMgr.h"

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

RoomMgr::RoomMgr(char *local_ip,
				 pj_uint32_t local_tcp_port,
				 pj_uint32_t local_udp_port,
				 pj_uint8_t  thread_pool_size)
	: Noncopyable()
	, local_tcp_sock_(-1)
	, local_udp_sock_(-1)
	, local_ip_(pj_str(local_ip))
	, local_tcp_port_(local_tcp_port)
	, local_udp_port_(local_udp_port)
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
	, tcp_thread_pool_(1)
	, udp_thread_pool_(thread_pool_size)
{
}

pj_status_t RoomMgr::Prepare()
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
	tcp_thread_pool_.Start();
	udp_thread_pool_.Start();

	return PJ_SUCCESS;
}

void RoomMgr::Destroy()
{
	active_ = PJ_FALSE;
	tcp_thread_pool_.Stop();
	udp_thread_pool_.Stop();
}

void RoomMgr::GetParamScene(const pj_uint8_t *storage,
						   Parameter *&param,
						   Scene *&scene,
						   Room *&room)
{
	pj_uint16_t type = *(pj_uint16_t *)(storage + (sizeof(pj_uint16_t) / sizeof(pj_uint8_t)));

	switch(type)
	{
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN:
		{
			param = new LoginParameter(storage);
			scene = new LoginScene();
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGOUT:
		{
			param = new LogoutParameter(storage);
			scene = new LogoutScene();
		    break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER:
		{
			param = new LinkRoomUserParameter(storage);
			room_map_t::iterator proom = rooms_.find(reinterpret_cast<LinkRoomUserParameter *>(param)->room_id_);
			room = proom->second;
			scene = new LinkRoomUserScene();
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_UNLINK_ROOM_USER:
		{
			param = new UnlinkRoomUserParameter(storage);
			room_map_t::iterator proom = rooms_.find(reinterpret_cast<UnlinkRoomUserParameter *>(param)->room_id_);
			room = proom->second;
			scene = new UnlinkRoomUserScene();
			break;
		}
	}
}

void RoomMgr::EventOnTcpAccept(evutil_socket_t fd, short event)
{
	pj_status_t status;
	pj_sock_t term_sock = PJ_INVALID_SOCKET;
	pj_sockaddr_t *term_addr = NULL;
	int *addr_len = NULL;

	status = pj_sock_accept(local_tcp_sock_, &term_sock, term_addr, addr_len);
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

	termination_map_t::mapped_type termination = new Termination(term_sock);
	pj_assert( termination != nullptr );

	terminations_.insert(termination_map_t::value_type(term_sock, termination));

	struct event *ev = event_new(evbase_, term_sock, EV_READ | EV_PERSIST, event_on_tcp_read, this);
	termination->tcp_ev_ = ev;

	int ret;
	ret = event_add(ev, NULL);
}

void RoomMgr::EventOnTcpRead(evutil_socket_t fd, short event)
{
	Parameter *param = NULL;
	Scene *scene = NULL;
	Room *room = NULL;
	pj_sock_t client_tcp_sock = fd;
	pj_status_t status;
	termination_map_t::iterator ptermination = terminations_.find(client_tcp_sock);
	termination_map_t::mapped_type termination = ptermination->second;

	RETURN_IF_FAIL( event & EV_READ );

	pj_ssize_t recvlen = MAX_STORAGE_SIZE - termination->tcp_storage_offset_;
	status = pj_sock_recv(client_tcp_sock,
		(char *)(termination->tcp_storage_ + termination->tcp_storage_offset_),
		&recvlen,
		0 );
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	if ( recvlen > 0 )
	{
		termination->tcp_storage_offset_ += recvlen;
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

			GetParamScene(termination->tcp_storage_, param, scene, room);

			tcp_thread_pool_.Schedule([=]()
			{
				scene->Maintain(param, termination, room);
				delete scene;
				delete param;
			});

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
		event_del(termination->tcp_ev_);
	}
	else /* if ( recvlen < 0 ) */
	{
		event_del(termination->tcp_ev_);
	}
}

void RoomMgr::EventOnUdpRead(evutil_socket_t fd, short event)
{
	pj_uint8_t datagram[MAX_UDP_DATA_SIZE];
	pj_ssize_t datalen = MAX_UDP_DATA_SIZE;
	pj_sock_t local_udp_sock = fd;
	pj_sockaddr_in addr;
	const pjmedia_rtp_hdr *rtp_hdr;
	pj_status_t status;
	const void *payload;
    unsigned payload_len;
	int addrlen = sizeof(addr);

	status = pj_sock_recvfrom(local_udp_sock, datagram, &datalen, 0, &addr, &addrlen);
	RETURN_IF_FAIL( status == PJ_SUCCESS );

	if ( datalen >= sizeof(*rtp_hdr) )
	{
		status = pjmedia_rtp_decode_rtp(&rtp_in_session_, 
				    datagram, (int)datalen, 
				    &rtp_hdr, &payload, &payload_len);
		RETURN_IF_FAIL( status == PJ_SUCCESS );

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
			event_base_dispatch(evbase_);
		}
	}
}

void RoomMgr::Foo(pj_str_t *ip, pj_uint16_t port)
{
	pj_uint8_t packet[MAX_UDP_DATA_SIZE];

	const pjmedia_rtp_hdr *hdr;
	const void *p_hdr;
	int hdrlen;
	pj_ssize_t size;

	pj_status_t status;
	status = pjmedia_rtp_encode_rtp (&rtp_out_session_, RTP_EXPAND_PAYLOAD_TYPE,
		0, sizeof(request_login_t), 160, &p_hdr, &hdrlen);
	RETURN_IF_FAIL( status == PJ_SUCCESS );
	
	request_login_t req_login;
	req_login.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LOGIN;
	req_login.proxy_id = 100;
	req_login.room_id = 462728;

	req_login.Serialize();

	hdr = (const pjmedia_rtp_hdr*) p_hdr;

	/* Copy RTP header to packet */
	pj_memcpy(packet, hdr, hdrlen);

	/* Zero the payload */
	pj_bzero(packet+hdrlen, sizeof(request_login_t));

	/* Send RTP packet */
	size = hdrlen + sizeof(request_login_t);

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, ip, port);
	pj_sock_sendto(local_udp_sock_, packet, &size, 0, &addr, sizeof(addr));
}
