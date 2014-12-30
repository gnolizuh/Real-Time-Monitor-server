#include "RoomMgr.h"

extern Config g_proxy_config;
extern SafeUdpSocket g_safe_client_sock;

void RoomMgr::event_func_proxy(evutil_socket_t fd, short event, void *arg)
{
	ev_function_t *pfunction = reinterpret_cast<ev_function_t *>(arg);
	(*pfunction)(fd, event, arg);
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
		g_safe_client_sock.Open(local_udp_sock_);
	} while(status != PJ_SUCCESS && (-- retrys > 0));
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	pj_caching_pool_init(&caching_pool_, &pj_pool_factory_default_policy, 0);

	pool_ = pj_pool_create(&caching_pool_.factory, "AvsProxyPool", 1000, 1000, NULL);

	status = log_open(pool_, log_file_name);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	status = pjmedia_rtp_session_init(&rtp_in_session_, RTP_EXPAND_PAYLOAD_TYPE, pj_rand());
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	status = pjmedia_rtp_session_init(&rtp_out_session_, 0, 0);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	evbase_ = event_base_new();
	RETURN_VAL_IF_FAIL( evbase_ != nullptr, -1 );

	ev_function_t function;
	function = std::bind(&RoomMgr::EventOnTcpAccept, this, std::placeholders::_1, std::placeholders::_2, nullptr);
	tcp_pfunction_ = new ev_function_t(function);
	tcp_ev_ = event_new(evbase_, local_tcp_sock_, EV_READ | EV_PERSIST, event_func_proxy, tcp_pfunction_);
	RETURN_VAL_IF_FAIL( tcp_ev_ != nullptr, -1 );

	function = std::bind(&RoomMgr::EventOnUdpRead, this, std::placeholders::_1, std::placeholders::_2, nullptr);
	udp_pfunction_ = new ev_function_t(function);
	udp_ev_ = event_new(evbase_, local_udp_sock_, EV_READ | EV_PERSIST, event_func_proxy, udp_pfunction_);
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
	mainloop_thread = thread(std::bind(&RoomMgr::MainloopThread, this));
	keepalive_thread = thread(std::bind(&RoomMgr::KeepaliveThread, this));
	sync_thread_pool_.Start();
	async_thread_pool_.Start();
	moMomgoDBClient.startup(g_proxy_config.mongodb_avsproxy_status_info_ip.ptr,g_proxy_config.mongodb_avsproxy_status_info_port,g_proxy_config.mongodb_avsproxy_status_info_ns.ptr);
	return PJ_SUCCESS;
}

void RoomMgr::Destroy()
{
	active_ = PJ_FALSE;
	//:D g_safe_client_sock.Close();
	sync_thread_pool_.Stop();
	async_thread_pool_.Stop();
	event_base_loopexit(evbase_, NULL);
	DELETE_MEMORY(tcp_pfunction_);
	DELETE_MEMORY(udp_pfunction_);

	pj_sock_close(local_tcp_sock_);
	pj_sock_close(local_udp_sock_);
	event_thread_.join();
	mainloop_thread.join();
	keepalive_thread.join();
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
	std::function<scene_opt_t (pj_buffer_t &)> maintain;
	TcpParameter *param = NULL;
	TcpScene     *scene = NULL;
	Room         *room  = NULL;
	pj_uint16_t type = (pj_uint16_t)ntohs(*(pj_uint16_t *)(storage + sizeof(param->length_)));

	switch(type)
	{
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN:
		{
			// Response the request and record the Client info.

			param = new LoginParameter(storage, storage_len);
			LoginScene *scene = new LoginScene();
			maintain = std::bind(&LoginScene::Maintain, shared_ptr<LoginScene>(scene), shared_ptr<TcpParameter>(param), termination);
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGOUT:
		{
			param = new LogoutParameter(storage, storage_len);
			scene = new LogoutScene();
			maintain = std::bind(&TcpScene::Maintain, shared_ptr<TcpScene>(scene), shared_ptr<TcpParameter>(param), termination, std::placeholders::_1);
		    break;
		}

		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM:
		{
			param = new LinkRoomParameter(storage, storage_len);
			CLinkRoomScene *scene = new CLinkRoomScene();
			// Dan:
			// Check the connection to the AVS for sending login request to AVS. # call RoomMgr::Login(2);
			// If the connection is established, return the Rooms Info. If not, send login request to AVS and then return the Rooms Info after AVS returning the Room Info as a responding.
			//
			maintain = std::bind(&CLinkRoomScene::Maintain, shared_ptr<CLinkRoomScene>(scene), shared_ptr<TcpParameter>(param), this, termination);
			break;

		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_UNLINK_ROOM:
		{
			param = new UnLinkRoomParameter(storage, storage_len);
			CUnLinkRoomScene *scene = new CUnLinkRoomScene();
			// Dan:
			// Check the size of Client list for free the memories. And no response for it.
			maintain = std::bind(&CUnLinkRoomScene::Maintain, shared_ptr<CUnLinkRoomScene>(scene), shared_ptr<TcpParameter>(param), this, termination);
		}
			break;
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_LINK_ROOM_USER:
		{
			param = new LinkRoomUserParameter(storage, storage_len);
			LinkRoomUserScene *scene = new LinkRoomUserScene();
			room  = GetRoom(reinterpret_cast<LinkRoomUserParameter *>(param)->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&LinkRoomUserScene::Maintain, shared_ptr<LinkRoomUserScene>(scene), shared_ptr<TcpParameter>(param), room, termination, std::placeholders::_1);
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_UNLINK_ROOM_USER:
		{
			param = new UnlinkRoomUserParameter(storage, storage_len);
			UnlinkRoomUserScene *scene = new UnlinkRoomUserScene();
			room  = GetRoom(reinterpret_cast<UnlinkRoomUserParameter *>(param)->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&UnlinkRoomUserScene::Maintain, shared_ptr<UnlinkRoomUserScene>(scene), shared_ptr<TcpParameter>(param), this, termination, std::placeholders::_1);
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_KEEP_ALIVE:
		{
			param = new KeepAliveParameter(storage, storage_len);
			scene = new KeepAliveScene();
			maintain = std::bind(&TcpScene::Maintain, shared_ptr<TcpScene>(scene), shared_ptr<TcpParameter>(param), termination, std::placeholders::_1);
			break;
		}
		default:
		{
				   return;
		}
	}

	sync_thread_pool_.Schedule(std::bind(&RoomMgr::Maintain, this, maintain));

	RETURN_IF_FAIL(type == REQUEST_FROM_CLIENT_TO_AVSPROXY_LOGIN);
	//sync_thread_pool_.Schedule(std::bind(&RoomMgr::SendRoomsInfo, this, termination));
}

void RoomMgr::UdpParamScene(const pj_uint8_t *storage,
							pj_uint16_t storage_len,
							pj_sockaddr_in& sockaddr_in
							)
{
	UdpParameter *param = NULL;
	UdpScene     *scene = NULL;
	pj_uint16_t type = ntohs(*(pj_uint16_t *)(storage));
	
	std::function<scene_opt_t(pj_buffer_t &)> maintain = nullptr;
	//PJ_LOG(5, ("RoomMgr", "UdpParamScene type.%d",type));
	switch(type)
	{
		// Dan:
		// Login Response.
		case REQUEST_FROM_AVS_TO_AVSPROXY_USERS_INFO:
		{
			//PJ_LOG(5, ("RoomMgr", "REQUEST_FROM_AVS_TO_AVSPROXY_USERS_INFO UdpParamScene type.%d", type));
			param = new UsersInfoParameter(storage, storage_len);
			UsersInfoScene * scene = new UsersInfoScene();
			maintain = std::bind(&UsersInfoScene::Maintain,
				shared_ptr<UsersInfoScene>(scene),
				shared_ptr<UdpParameter>(param),
				this,
				std::placeholders::_1);
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_MOD_USER_MEDIA:
		{
			/* Proxy在收到Avs发来的ModUser之后, 需要更新所有在线的Client */
			param = new ModUserMediaParameter(storage, storage_len);
			scene = new ModUserMediaScene();
			Room *room = GetRoom(param->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&UdpScene::Maintain,
				shared_ptr<UdpScene>(scene),
				shared_ptr<UdpParameter>(param),
				room,
				std::placeholders::_1);
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_ADD_USER:
		{
			/* Proxy在收到Avs发来的AddUser之后, 需要更新所有在线的Client */
			param = new AddUserParameter(storage, storage_len);
			scene = new AddUserScene();
			Room *room = GetRoom(param->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&UdpScene::Maintain,
				shared_ptr<UdpScene>(scene),
				shared_ptr<UdpParameter>(param),
				room,
				std::placeholders::_1);
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_DEL_USER:
		{
			/* Proxy在收到Avs发来的DelUser之后, 需要更新所有在线的Client */
			param = new DelUserParameter(storage, storage_len);
			scene = new DelUserScene();
			Room *room = GetRoom(param->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&UdpScene::Maintain,
				shared_ptr<UdpScene>(scene),
				shared_ptr<UdpParameter>(param),
				room,
				std::placeholders::_1);
			break;
		}
		case REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM:
		{
			/* Proxy在收到Avs发来的RTP包之后, 需要向所有在线的Client转发 */
			param = new RTPParameter(storage, storage_len);
			RTPScene* scene = new RTPScene();
			//Room *room = GetRoom(param->room_id_);
			//if (room == NULL) return;
			maintain = std::bind(&RTPScene::Maintain,
				shared_ptr<RTPScene>(scene),
				shared_ptr<UdpParameter>(param),
				this
				);
			break;
		}
		case RESPONSE_FROM_AVS_TO_AVSPROXY_LOGIN:
		{
			param = new ResLoginParameter(storage, storage_len);
			scene = new ResLoginScene();
			Room *room = GetRoom(param->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&UdpScene::Maintain,
				shared_ptr<UdpScene>(scene),
				shared_ptr<UdpParameter>(param),
				room,
				std::placeholders::_1);
			break;
		}
		case RESPONSE_FROM_AVS_TO_AVSPROXY_KEEP_ALIVE:
		{
			param = new ResKeepAliveParameter(storage, storage_len);
			scene = new ResKeepAliveScene();
			Room *room = GetRoom(param->room_id_);
			if (room == NULL) return;
			maintain = std::bind(&UdpScene::Maintain,
				shared_ptr<UdpScene>(scene),
				shared_ptr<UdpParameter>(param),
				room,
				std::placeholders::_1);
			break;
		}
			//Added by Dan.
		case REPORT_FROM_AVS_TO_AVSPROXY:
		{
			param = new CRecvAVSStatusParameter(storage, storage_len);
			CRecvAVSStatusInfo* scene = new CRecvAVSStatusInfo();
			maintain = std::bind(&CRecvAVSStatusInfo::Maintain,
				shared_ptr<CRecvAVSStatusInfo>(scene),
				shared_ptr<UdpParameter>(param),
				this);
			break;
		}
		case REQUEST_FROM_CLIENT_TO_AVSPROXY_NAT:
		{
			param = new NATforClientParam(storage, storage_len);
			CNATforClient* scene = new CNATforClient();
			maintain = std::bind(&CNATforClient::Maintain,
				shared_ptr<CNATforClient>(scene),
				shared_ptr<UdpParameter>(param),
				this, 
				sockaddr_in);
		}
			break;
		default:
		{
				   return;
		}
			
	}

	
	PoolThread<std::function<void ()>> &thread_pool =
		(type == REQUEST_FROM_AVS_TO_AVSPROXY_MEDIA_STREAM) ? async_thread_pool_ : sync_thread_pool_;
	thread_pool.Schedule(std::bind(&RoomMgr::Maintain, this, maintain));
}

void RoomMgr::Maintain(std::function<scene_opt_t (pj_buffer_t &)> &maintain)
{
	pj_buffer_t buffer;
	switch(maintain(buffer))
	{
		case SCENE_OPT_TCP_TO_CLIENT:
		{
			pj_uint16_t client_type = *(pj_uint16_t *)&buffer[2];
			switch(ntohs(client_type))
			{
				case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_ADD_USER:
				case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_DEL_USER:
				case REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_MOD_MEDIA:
				{
					SendTCPPacketToAllClient(buffer);
				}
					break;
				default:
					break;
			}
		}
			break;
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
					break;
				default:
					break;
			}
		}
			break;
		default:
			break;
	}
}

pj_status_t RoomMgr::AddTermination(pj_sock_t fd)
{
	lock_guard<mutex> lock(terminations_lock_);
	termination_map_t::iterator ptermination = terminations_.find(fd);
	RETURN_VAL_IF_FAIL( ptermination == terminations_.end(), PJ_EEXISTS );

	termination_map_t::mapped_type termination = new Termination(fd);
	pj_assert( termination != nullptr );

	terminations_.insert(termination_map_t::value_type(fd, termination));

	ev_function_t function;
	ev_function_t *pfunction = nullptr;

	function = std::bind(&RoomMgr::EventOnTcpRead, this, std::placeholders::_1, std::placeholders::_2, termination);
	pfunction = new ev_function_t(function);
	termination->pfunction_ = pfunction;

	struct event *ev = event_new(evbase_, fd, EV_READ | EV_PERSIST, event_func_proxy, pfunction);
	termination->tcp_ev_ = ev;

	int evret;
	evret = event_add(ev, NULL);

	return PJ_SUCCESS;
}

bool RoomMgr::UpdateClientRoomFollows(pj_uint32_t client_id, pj_uint16_t port, char* p)
{
	bool booRet = false;
	//mtu32list		loroomlist;
	//{
	//	lock_guard<mutex> lock(moRoomIDtoClientidwfdlist_lock_);
	//	mtu32wu32tou32mapItr	lItr = moRoomIDtoClientidwfdlist.begin();
	//	while (lItr != moRoomIDtoClientidwfdlist.end())
	//	{
	//		mtu32tou32mapItr lItr_se = lItr->second.find(client_id);
	//		if (lItr_se != lItr->second.end())
	//		{
	//			loroomlist.push_back(lItr->first);
	//		}
	//	}
	//}
	lock_guard<mutex> lock(moClientidtoRoomList_lock_);
	mtu32tou32ListItr lItr = moClientidtoRoomList.find(client_id);
	mtu32setItr lItr_room = lItr->second.begin();
	while (lItr_room != lItr->second.end())
	{
		Room* lproom = GetRoom(*lItr_room);
		if (lproom != NULL)
		{
			
		}
		++lItr_room;
	}
	return booRet;
}

bool RoomMgr::UpdateTerminationUdpPort(pj_uint32_t client_id, pj_uint16_t port, char* ip)
{
	//PJ_LOG(5, ("RoomMgr", "UpdateTerminationUdpPort"));
	bool booRet = false;
	lock_guard<mutex> lock(terminations_lock_);
	termination_map_t::iterator ptermination = terminations_.begin();
	while (ptermination != terminations_.end())
	{
		if (ptermination->second->client_id_ == client_id)
		{
			if (ptermination->second->ip_.ptr != NULL)
			{
				free(ptermination->second->ip_.ptr);
			}
			ptermination->second->ip_ = pj_str(strdup(ip));
			ptermination->second->media_port_ = port;
			booRet = true;
			PJ_LOG(5, ("RoomMgr", "UpdateTerminationUdpPort IP %s Port %d.", ptermination->second->ip_.ptr, ptermination->second->media_port_));
			break;
		}
		++ptermination;
	}
	return booRet;
}

pj_status_t RoomMgr::DelTermination(pj_sock_t fd)
{
	lock_guard<mutex> lock(terminations_lock_);
	termination_map_t::iterator ptermination = terminations_.find(fd);
	RETURN_VAL_IF_FAIL( ptermination != terminations_.end(), PJ_ENOTFOUND );

	termination_map_t::mapped_type termination = ptermination->second;
	terminations_.erase(ptermination); // First free the memory of iterator, termination is safe.

	RETURN_VAL_IF_FAIL( termination != nullptr, PJ_SUCCESS );

	pj_sock_close(termination->tcp_socket_);
	event_del(termination->tcp_ev_);
	event_free(termination->tcp_ev_);

	DisconnectScene *scene = new DisconnectScene();
	std::function<scene_opt_t (pj_buffer_t &)> maintain = std::bind(&DisconnectScene::Maintain, scene, termination, this);
	sync_thread_pool_.Schedule(std::bind(&RoomMgr::Maintain, this, maintain));

	return PJ_SUCCESS;
}

void RoomMgr::EventOnTcpAccept(evutil_socket_t fd, short event, void *arg)
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

	status = AddTermination(term_sock);
	RETURN_IF_FAIL( status == PJ_SUCCESS );
}

void RoomMgr::EventOnTcpRead(evutil_socket_t fd, short event, void *arg)
{
	pj_sock_t client_tcp_sock = fd;
	pj_status_t status;
	
	termination_map_t::mapped_type termination = reinterpret_cast<termination_map_t::mapped_type>(arg);
	RETURN_IF_FAIL(termination != nullptr);
	RETURN_IF_FAIL(event & EV_READ);

	pj_ssize_t recvlen = MAX_STORAGE_SIZE - termination->tcp_storage_offset_;
	status = pj_sock_recv(client_tcp_sock,
		(char *)(termination->tcp_storage_ + termination->tcp_storage_offset_),
		&recvlen,
		0 );

	if ( recvlen > 0 )
	{
		termination->tcp_storage_offset_ += (pj_uint16_t)recvlen;
		do {
			RETURN_IF_FAIL(termination->tcp_storage_offset_ >= sizeof(pj_uint16_t));

			pj_uint16_t packet_len = ntohs(*(pj_uint16_t *)termination->tcp_storage_);
			pj_uint16_t total_len  = packet_len + sizeof(packet_len);

			if ( total_len > MAX_STORAGE_SIZE )
			{
				termination->tcp_storage_offset_ = 0;
				break;
			}
			else if ( total_len > termination->tcp_storage_offset_ )
			{
				break;
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
		} while (1);
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

void RoomMgr::EventOnUdpRead(evutil_socket_t fd, short event, void *arg)
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

		UdpParamScene(payload, payload_len, addr);

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

void RoomMgr::MainloopThread()
{
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;

	if (!pj_thread_is_registered())
	{
		if (pj_thread_register(NULL, rtpdesc, &thread) == PJ_SUCCESS)
		{
			pj_uint64_t lu64curtime = GetTickCount64();
			pj_uint64_t lu64reporttomongotime = lu64curtime;
			pj_uint64_t lu64reconntomongotime = lu64curtime;
			pj_uint64_t lu64recheckroom_list = lu64curtime;
			while (active_)
			{
				lu64curtime = GetTickCount64();
				if (lu64curtime - lu64recheckroom_list >= g_proxy_config.mongodb_avsproxy_status_info_rcheck_time_cycle)
				{
					PJ_LOG(5, ("RoomMgr", "Re-Check AVS status info begin."));
					lu64recheckroom_list = lu64curtime;
					CheckAvsStatusInfo(lu64curtime, g_proxy_config.mongodb_avsproxy_status_info_rcheck_time_gap);
					PJ_LOG(5, ("RoomMgr", "Re-Check end."));
				}
				if (lu64curtime - lu64reporttomongotime >= g_proxy_config.mongodb_avsproxy_status_info_report_time_cycle)
				{
					PJ_LOG(5, ("RoomMgr", "Report AVS status info begin."));
					lu64reporttomongotime = lu64curtime;
					ReportToMongoDB();
					PJ_LOG(5, ("RoomMgr", "Report end."));
				}
				if (lu64curtime - lu64reconntomongotime >= g_proxy_config.mongodb_avsproxy_status_info_reconn_time_cycle)
				{
					PJ_LOG(5, ("RoomMgr", "Check MongoDB connections begin."));
					lu64reconntomongotime = lu64curtime;
					if (moMomgoDBClient.checkconnection() < 0)
					{
						if (moMomgoDBClient.reconnect() < 0)
						{
							//log
							PJ_LOG(5, ("RoomMgr", "MongoDB reconnecting failed!"));
						}
						else
						{
							//log
							PJ_LOG(5, ("RoomMgr", "MongoDB reconnecting success."));
						}
					}
					PJ_LOG(5, ("RoomMgr", "Check MongoDB connections end."));
				}
				Sleep(100);
			}
		}
	}
}

void RoomMgr::KeepaliveThread()
{
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;

	if (!pj_thread_is_registered())
	{
		if (pj_thread_register(NULL, rtpdesc, &thread) == PJ_SUCCESS)
		{
			pj_uint64_t lu64curtime = GetTickCount64();
			pj_uint64_t lu64prekeepalivetime = lu64curtime;
			while (active_)
			{
				lu64curtime = GetTickCount64();
				if (lu64curtime - lu64prekeepalivetime >= g_proxy_config.keepalive_time)
				{
					lu64prekeepalivetime = lu64curtime;
					

					SendKeepalivePacketToAllAvs();
				}
				Sleep(100);
			}
		}
	}
}
#if 0
void RoomMgr::SendRoomsInfo(Termination *termination)
{
	RETURN_IF_FAIL(termination);

	lock_guard<mutex> lock(rooms_lock_);

	request_to_client_rooms_info_t rooms_info;
	rooms_info.client_request_type = REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOMS_INFO;
	rooms_info.proxy_id = g_proxy_config.proxy_id;
	rooms_info.client_id = termination->GetClientID();
	rooms_info.room_count = rooms_.size();

	int i = 0;
	for(room_map_t::iterator proom = rooms_.begin();
		proom != rooms_.end();
		++ proom, ++ i)
	{
		room_map_t::key_type room_id = proom->first;
		room_map_t::mapped_type room = proom->second;

		room_info_t room_info;
		room_info.room_id = room_id;
		room_info.user_count = room->GetUsers().size();
		int j = 0;
		for(user_map_t::iterator puser = room->GetUsers().begin();
			puser != room->GetUsers().end();
			++ puser, ++ j)
		{
			user_map_t::key_type user_id = puser->first;
			user_map_t::mapped_type user = puser->second;

			user_info_t user_info;
			user_info.user_id = user_id;
			user_info.audio_ssrc = user->audio_ssrc_;
			user_info.video_ssrc = user->video_ssrc_;
			room_info.users.push_back(user_info);
		}
		rooms_info.rooms.push_back(room_info);
	}

	rooms_info.Serialize();

	pj_ssize_t sndlen = rooms_info.Size();
	std::unique_ptr<pj_uint8_t> buf(new pj_uint8_t[sndlen]);
	if(buf)
	{
		rooms_info.Copy(buf.get(), sndlen);
		termination->SendTCPPacket(buf.get(), &sndlen);
	}
}
#endif
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
			termination->SendTCPPacket(&buffer[0], &sndlen);
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
			room->SendRTPPacket(&buffer[0], &sndlen);
		}
	}
}

void RoomMgr::SendKeepalivePacketToAllAvs()
{
	lock_guard<mutex> lock(rooms_lock_);
	room_map_t::iterator proom = rooms_.begin();
	for (; proom != rooms_.end(); ++proom)
	{
		room_map_t::mapped_type room = proom->second;
		if (room != nullptr)
		{
			if (proom->second->GetCurRoomState() == Room::_enAvaliable)
			{
				pj_buffer_t buffer;
				request_to_avs_keep_alive_t lopack;
				lopack.proxy_id = g_proxy_config.proxy_id;
				lopack.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_KEEP_ALIVE;
				lopack.room_id = proom->first;
				pj_uint8_t* lppack = (pj_uint8_t*)&lopack;
				buffer.assign(lppack, lppack + sizeof(lopack));
				pj_ssize_t sndlen = buffer.size();
				room->SendRTPPacket(&buffer[0], &sndlen);
			}
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
	return g_safe_client_sock.Sendto(packet, &size, ip, port);
	//return pj_sock_sendto(local_udp_sock_, packet, &size, 0, &addr, sizeof(addr));
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
		room = new Room( room_id, *ip, port, local_udp_sock_, rtp_out_session_, rtp_out_sess_lock_);
		pj_assert( room != nullptr );
		room->SetRoomState(Room::_enBuilding);
		room->Prepare();
		room->Launch();
		rooms_[room_id] = room;
	}
	else
	{
		room = proom->second;
	}
	//rooms_[room_id] = room;
	rooms_lock_.unlock();

	pj_ssize_t sndlen = sizeof(req_login);
	pj_status_t	lostatus	=	room->SendRTPPacket(&req_login, &sndlen);

	PJ_LOG(5, ("RoomMgr", "Login room status %d.", lostatus));

	return lostatus;
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

void RoomMgr::OnConsole(stringstream &output)
{
	output << "\nStatistics:\n";
	{
		lock_guard<mutex> lock(terminations_lock_);
		termination_map_t::iterator ptermination = terminations_.begin();
		for(; ptermination != terminations_.end(); ++ ptermination)
		{
			termination_map_t::mapped_type termination = ptermination->second;
			if(termination != nullptr)
			{
				termination->OnConsole(output);
			}
		}
	}

	{
		lock_guard<mutex> lock(rooms_lock_);
		room_map_t rooms_;
		room_map_t::iterator proom = rooms_.begin();
		for(; proom != rooms_.end(); ++ proom)
		{
			room_map_t::mapped_type room = proom->second;
			if(room != nullptr)
			{
				room->OnConsole(output);
			}
		}
	}
}

void RoomMgr::UpdateAvsStatusInfo(pj_uint32_t iu32roomid, pj_uint32_t iu32avsip, pj_uint16_t iu16avsport, pj_uint64_t iu64timestamp )
{
	in_addr laddr;
	laddr.S_un.S_addr = htonl(iu32avsip);
	lock_guard<mutex> lock(moAvsStatusInfo_lock_);
	mtu32toMapu32wu16tou64Itr lItr = moAvsStatusInfo.find(iu32roomid);
	if (lItr != moAvsStatusInfo.end())
	{
		mtu32wu16	loIPwPort(iu32avsip, iu16avsport);
		mtu32wu16tou64Itr lItr_ipt = lItr->second.find(loIPwPort);
		if (lItr_ipt != lItr->second.end())
		{
			lItr_ipt->second = iu64timestamp;
			PJ_LOG(5, ("RoomMgr", "UpdateAvsStatusInfo. Room and AVS have already been existing. AVS RoomID %d IP %s Port %d.", iu32roomid, inet_ntoa(laddr), iu16avsport));
		}
		else
		{
			mtu32wu16wu64	loIPwPortwTimestamp(loIPwPort, iu64timestamp);
			lItr->second.insert(loIPwPortwTimestamp);
			PJ_LOG(5, ("RoomMgr", "UpdateAvsStatusInfo. Room has already been existing. New AVS RoomID %d IP %s Port %d.", iu32roomid, inet_ntoa(laddr), iu16avsport));
		}
	}
	else
	{
		mtu32wu16	loIPwPort(iu32avsip, iu16avsport);
		mtu32wu16wu64	loIPwPortwTimestamp(loIPwPort, iu64timestamp);
		


		PJ_LOG(5, ("RoomMgr", "UpdateAvsStatusInfo. New AVS RoomID %d IP %s Port %d.", iu32roomid, inet_ntoa(laddr), iu16avsport));
		moAvsStatusInfo[iu32roomid].insert(loIPwPortwTimestamp);
	}
	
}

void RoomMgr::CheckAvsStatusInfo( pj_uint64_t iu64curtime, pj_uint64_t iu64timegap )
{
	lock_guard<mutex> lock(moAvsStatusInfo_lock_);
	mtu32toMapu32wu16tou64Itr	lItr	=	moAvsStatusInfo.begin();
	while (lItr != moAvsStatusInfo.end())
	{
		mtu32wu16tou64Itr	lItr_ippt = lItr->second.begin();
		while (lItr_ippt != lItr->second.end())
		{
			if (iu64curtime - lItr_ippt->second >= iu64timegap)
			{
				in_addr laddr;
				laddr.S_un.S_addr = htonl(lItr_ippt->first.first);
				PJ_LOG(5, ("RoomMgr", "Time Out. AVS RoomID %d IP %s Status Info is unavalible and Removed.", lItr->first, inet_ntoa(laddr)));
				lItr->second.erase(lItr_ippt++);
			}
			else
			{
				in_addr laddr;
				laddr.S_un.S_addr = htonl(lItr_ippt->first.first);
				PJ_LOG(5, ("RoomMgr", "\tAVS RoomID %d IP %s Port %d.", lItr->first, inet_ntoa(laddr), lItr_ippt->first.second));
				lItr_ippt->second = iu64curtime;
				++lItr_ippt;
			}
		}
		if (lItr->second.size() == 0)
		{
			PJ_LOG(5, ("RoomMgr", "There is not any AVS Status Info Reporting for RoomID %d.", lItr->first));
			moAvsStatusInfo.erase(lItr++);
		}
		else
		{
			++lItr;
		}
	}
}

bool RoomMgr::GetAvsStatusInfo(pj_uint32_t iu32roomid, pj_uint32_t& ou32avsip, pj_uint16_t& ou16avsport)
{
	bool	booRet = false;
	lock_guard<mutex> lock(moAvsStatusInfo_lock_);
	mtu32toMapu32wu16tou64Itr	lItr = moAvsStatusInfo.find(iu32roomid);
	if (lItr != moAvsStatusInfo.end())
	{
		if (lItr->second.size() > 0)
		{
			ou32avsip	=	lItr->second.begin()->first.first;
			ou16avsport	=	lItr->second.begin()->first.second;
			booRet = true;
		}
	}
	return booRet;
}

void RoomMgr::ReportToMongoDB()
{
	mtu32toMapu32wu16tou64	loavsstatusinfo;
	{
		lock_guard<mutex> lock(moAvsStatusInfo_lock_);
		loavsstatusinfo = moAvsStatusInfo;
	}
	bson	locond[1];
	bson	loop[1];
	bson_init(locond);
	bson_append_int(locond, MDB_TABLE_COL_RRTVMSS_ID_TINT, g_proxy_config.proxy_id);
	bson_finish(locond);
	bson_init(loop);
	bson_append_start_object(loop, MDB_OP_SET);
	bson_append_string(loop, MDB_TABLE_COL_RRTVMSS_IP_TSTR, g_proxy_config.local_ip.ptr);
	bson_append_int(loop, MDB_TABLE_COL_RRTVMSS_PORT_TINT, g_proxy_config.local_tcp_port);
	bson_append_int(loop, MDB_TABLE_COL_RRTVMSS_UPORT_TINT, g_proxy_config.local_udp_port);
	bson_append_start_array(loop, MDB_TABLE_COL_ROOMLIST_TARY);

	char lacindex[10] = { 0 };
	pj_uint32_t	lu32index = 0;//	=	loavsstatusinfo.size();
	mtu32toMapu32wu16tou64Itr lItr = loavsstatusinfo.begin();
	while (lItr != loavsstatusinfo.end())
	{
		pj_uint32_t	lu32roomid = lItr++->first;
		memset(lacindex, 0, sizeof(lacindex));
		bson_append_int(loop, itoa(lu32index++, lacindex, 10), lu32roomid);
	}
	bson_append_finish_array(loop);
	bson_append_finish_object(loop);
	bson_finish(loop);
	if (!moMomgoDBClient.upsert(locond, loop))
	{
		//log
		PJ_LOG(5, ("RoomMgr", "Upsert data to MongoDB failed"));
	}
}


void RoomMgr::UpdateRoomClientList(pj_uint32_t room_id, pj_uint32_t clientid, pj_uint32_t fd)
{
	lock_guard<mutex> lock(moRoomIDtoClientidwfdlist_lock_);
	mtu32wu32tou32mapItr	lItr = moRoomIDtoClientidwfdlist.find(room_id);
	if (lItr == moRoomIDtoClientidwfdlist.end())
	{
			
		mtu32tou32map		losocketlist;
		losocketlist.insert(mtu32tou32pair(clientid,fd));
		moRoomIDtoClientidwfdlist.insert(mtu32wu32tou32pair(room_id, losocketlist));
	}
	else
	{
		
		mtu32tou32mapItr lItr_se = lItr->second.find(clientid);
		if (lItr_se != lItr->second.end())
		{
			lItr_se->second = fd;
		}
		else
		{
			lItr->second.insert(mtu32tou32pair(clientid, fd));
		}
	}

}

bool RoomMgr::DeleteRoomClient(pj_uint32_t room_id, pj_uint32_t clientid)
{
	bool	booRet = false;
	lock_guard<mutex> lock(moRoomIDtoClientidwfdlist_lock_);
	mtu32wu32tou32mapItr	lItr = moRoomIDtoClientidwfdlist.find(room_id);
	if (lItr == moRoomIDtoClientidwfdlist.end())
	{
	}
	else
	{
		mtu32tou32mapItr lItr_clientidfd = lItr->second.find(clientid);
		if (lItr_clientidfd == lItr->second.end())
		{
		}
		else
		{
			lItr->second.erase(lItr_clientidfd);
		}
		if (lItr->second.size() == 0)
		{
			moRoomIDtoClientidwfdlist.erase(lItr);
			booRet = true;// Delete room
		}
	}
	return booRet;
}

bool RoomMgr::DeleteRoomClient_Sockfd(pj_uint32_t clsockfd, pj_uint32_t &client_id, RoomMgr::mtu32list& roomlist)
{
	bool booRet = false;
	lock_guard<mutex> lock(moRoomIDtoClientidwfdlist_lock_);
	mtu32wu32tou32mapItr	lItr = moRoomIDtoClientidwfdlist.begin();
	while (lItr != moRoomIDtoClientidwfdlist.end())
	{
		mtu32tou32mapItr lItr_clientidfd = lItr->second.begin();
		while (lItr_clientidfd != lItr->second.end())
		{
			if (lItr_clientidfd->second == clsockfd)
			{
				client_id = lItr->first;
				lItr->second.erase(lItr_clientidfd++);
			}
			else
			{
				++lItr_clientidfd;
			}
		}
		if (lItr->second.size() == 0)
		{
			roomlist.push_back(lItr->first);
			moRoomIDtoClientidwfdlist.erase(lItr++);
			booRet = true;
		}
		else
		{
			++lItr;
		}
		
	}
	return booRet;
}

void RoomMgr::UpdateClientRoomList(pj_uint32_t clientid, pj_uint32_t room_id)
{
	lock_guard<mutex> lock(moClientidtoRoomList_lock_);
	mtu32tou32ListItr lItr = moClientidtoRoomList.find(clientid);
	if (lItr == moClientidtoRoomList.end())
	{
		mtu32set		loroomlist;
		loroomlist.insert(room_id);
		moClientidtoRoomList.insert(mtu32wu32List(clientid, loroomlist));
	}
	else
	{
		lItr->second.insert(room_id);
	}
}

bool RoomMgr::DeleteClientRoom(pj_uint32_t clientid, pj_uint32_t room_id)
{
	bool booRet = false;
	lock_guard<mutex> lock(moClientidtoRoomList_lock_);
	mtu32tou32ListItr	lItr = moClientidtoRoomList.find(clientid);
	if (lItr == moClientidtoRoomList.end())
	{
	}
	else
	{
		mtu32setItr	lItr_roomid = lItr->second.find(room_id);
		if (lItr_roomid == lItr->second.end())
		{
		}
		else
		{
			lItr->second.erase(lItr_roomid);
		}
		if (lItr->second.size() == 0)
		{
			moClientidtoRoomList.erase(lItr);
			booRet = true;
		}
	}
	
	return booRet;
}

void RoomMgr::SendRoomUserInfo(pj_uint32_t roomid)
{
	Room* lproom = GetRoom(roomid);
	if (lproom == NULL)
	{
		return;
	}
	room_info_t	loroominfo;
	lproom->GetRoomInfo(loroominfo);

	lock_guard<mutex> lock(moRoomIDtoClientidwfdlist_lock_);
	mtu32wu32tou32mapItr	lItr = moRoomIDtoClientidwfdlist.find(roomid);
	
	if (lItr != moRoomIDtoClientidwfdlist.end())
	{
		mtu32tou32mapItr lItr_client = lItr->second.begin();
		while (lItr_client != lItr->second.end())
		{
			request_to_client_rooms_info_t rooms_info;
			rooms_info.client_request_type = REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOMS_INFO;
			rooms_info.proxy_id = g_proxy_config.proxy_id;
			rooms_info.client_id = lItr_client->first;
			rooms_info.room_count = 1;
			rooms_info.rooms.push_back(loroominfo);
			rooms_info.Serialize();
			{

				lock_guard<mutex> lock(terminations_lock_);
				termination_map_t::iterator ptermination = terminations_.find(lItr_client->second);
				if (ptermination != terminations_.end())
				{
					pj_ssize_t sndlen = rooms_info.Size();
					std::unique_ptr<pj_uint8_t> buf(new pj_uint8_t[sndlen]);
					if (buf)
					{
						if (ptermination->second != NULL)
						{
							rooms_info.Copy(buf.get(), sndlen);
							if (ptermination->second->SendTCPPacket(buf.get(), &sndlen) != PJ_SUCCESS)
							{
								PJ_LOG(5, ("RoomMgr", "Error! Could not send the room %d info.", roomid));
							}
						}
					}
				}

			}
			++lItr_client;
		}
	}
	
}



