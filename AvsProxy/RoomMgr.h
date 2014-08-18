#ifndef __AVS_PROXY_ROOM_MGR__
#define __AVS_PROXY_ROOM_MGR__

#include <map>
#include <thread>
#include <iostream>

#include "Com.h"
#include "Room.h"
#include "Parameter.h"
#include "LoginScene.h"
#include "LogoutScene.h"
#include "LinkRoomUserScene.h"
#include "UnlinkRoomUserScene.h"
#include "UsersInfoScene.h"
#include "ModUserMediaScene.h"
#include "AddUserScene.h"
#include "DelUserScene.h"
#include "KeepAliveScene.h"
#include "ResKeepAliveScene.h"
#include "ResLoginScene.h"
#include "RTPScene.h"
#include "AvsProxyStructs.h"
#include "PoolThread.hpp"

using std::map;
using std::thread;

class Package
{
public:
	Package(int idx)
		: idx_ (idx)
	{
	}

	void foo() const { std::cout << "foo(" << idx_ << ")" << "\n"; }

private:
	int idx_;
};

typedef map<pj_sock_t, Termination *> termination_map_t;
typedef map<pj_uint32_t, Room *>      room_map_t;

class RoomMgr
	: public Noncopyable
{
public:
	RoomMgr(const pj_str_t &, pj_uint16_t, pj_uint16_t, pj_uint8_t);

	pj_status_t Prepare(const pj_str_t &);
	pj_status_t Launch();
	void        Destroy();
	void        Login(pj_str_t *, pj_uint16_t, pj_int32_t);
	void        Logout(pj_str_t *, pj_uint16_t, pj_int32_t);
	room_map_t::mapped_type GetRoom(room_map_t::key_type);

protected:
	pj_status_t AddTermination(const pj_str_t &, pj_sock_t);
	pj_status_t DelTermination(pj_sock_t);

private:
	static void event_on_tcp_accept(evutil_socket_t, short, void *);
	static void event_on_tcp_read(evutil_socket_t, short, void *);
	static void event_on_udp_read(evutil_socket_t, short, void *);

	void TcpParamScene(Termination *, const pj_uint8_t *, pj_uint16_t);
	void UdpParamScene(const pj_uint8_t *, pj_uint16_t);
	void EventOnTcpAccept(evutil_socket_t, short);
	void EventOnTcpRead(evutil_socket_t, short);
	void EventOnUdpRead(evutil_socket_t, short);
	void EventThread();

private:
	pj_sock_t          local_tcp_sock_;
	pj_sock_t          local_udp_sock_;
	pj_str_t           local_ip_;
	pj_uint16_t        local_tcp_port_;
	pj_uint16_t        local_udp_port_;
	pj_caching_pool    caching_pool_;
	pj_pool_t		  *pool_;
	pj_uint8_t         thread_pool_size_;
	thread             event_thread_;
	struct event      *tcp_ev_, *udp_ev_;
	struct event_base *evbase_;
	pj_bool_t          active_;
	termination_map_t  terminations_;
	room_map_t         rooms_;
	pjmedia_rtp_session rtp_in_session_;
	pjmedia_rtp_session rtp_out_session_;
	PoolThread<std::function<void ()>> sync_thread_pool_;
	PoolThread<std::function<void ()>> async_thread_pool_;
};

#endif
