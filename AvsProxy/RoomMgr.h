#ifndef __AVS_PROXY_ROOM_MGR__
#define __AVS_PROXY_ROOM_MGR__

#include <map>
#include <set>
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
#include "DisconnectScene.h"
#include "AvsProxyStructs.h"
#include "PoolThread.hpp"
#include "Config.h"
#include "RecvAVSStatusInfo.h"
#include "LinkRoomScene.h"
#include "UnLinkRoomScene.h"
#include "CMongoClient.h"
#include "NATforClient.h"
using std::map;
using std::thread;

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
	pj_status_t Login(pj_str_t *, pj_uint16_t, pj_int32_t);
	pj_status_t Logout(pj_str_t *, pj_uint16_t, pj_int32_t);
	void        OnConsole(stringstream &output);
	room_map_t::mapped_type GetRoom(room_map_t::key_type);
	void        SendRoomsInfo(Termination *);
	void        SendTCPPacketToAllClient(pj_buffer_t &);
	void        SendRTPPacketToAllAvs(pj_buffer_t &);
	pj_status_t SendRTPPacket(const pj_str_t &, pj_uint16_t, int, void *, int);
	void		SendKeepalivePacketToAllAvs();

public:
	void		UpdateAvsStatusInfo(pj_uint32_t, pj_uint32_t, pj_uint16_t, pj_uint64_t);
	void		CheckAvsStatusInfo(pj_uint64_t, pj_uint64_t);
	bool		GetAvsStatusInfo(pj_uint32_t, pj_uint32_t&, pj_uint16_t&);
	void		ReportToMongoDB();
	void		UpdateRoomClientList(pj_uint32_t, pj_uint32_t, pj_uint32_t);
	bool		DeleteRoomClient(pj_uint32_t, pj_uint32_t);
	typedef std::list<pj_uint32_t>		mtu32list;
	typedef mtu32list::iterator			mtu32listItr;
	bool		DeleteRoomClient_Sockfd(pj_uint32_t, pj_uint32_t&, mtu32list&);
	void		UpdateClientRoomList(pj_uint32_t, pj_uint32_t);
	bool		DeleteClientRoom(pj_uint32_t, pj_uint32_t);
	void		SendRoomUserInfo(pj_uint32_t);
	bool		UpdateTerminationUdpPort(pj_uint32_t, pj_uint16_t, char*);
	bool		UpdateClientRoomFollows(pj_uint32_t, pj_uint16_t, char*);
	
protected:
	pj_status_t AddTermination(pj_sock_t);
	pj_status_t DelTermination(pj_sock_t);

private:
	static void event_func_proxy(evutil_socket_t, short, void *);

	void TcpParamScene(Termination *, const pj_uint8_t *, pj_uint16_t);
	void UdpParamScene(const pj_uint8_t *, pj_uint16_t, pj_sockaddr_in&);
	void Maintain(std::function<scene_opt_t(pj_buffer_t &)> &maintain);
	void EventOnTcpAccept(evutil_socket_t fd, short event, void *arg);
	void EventOnTcpRead(evutil_socket_t fd, short event, void *arg);
	void EventOnUdpRead(evutil_socket_t fd, short event, void *arg);
	void EventThread();
	void MainloopThread();
	void KeepaliveThread();
private:
	pj_sock_t           local_tcp_sock_;
	pj_sock_t           local_udp_sock_;
	pj_str_t            local_ip_;
	pj_uint16_t         local_tcp_port_;
	pj_uint16_t         local_udp_port_;
	pj_caching_pool     caching_pool_;
	ev_function_t      *tcp_pfunction_;
	ev_function_t      *udp_pfunction_;
	pj_pool_t		   *pool_;
	pj_uint8_t          thread_pool_size_;
	thread              event_thread_;
	thread				mainloop_thread;
	thread				keepalive_thread;
	struct event       *tcp_ev_, *udp_ev_;
	struct event_base  *evbase_;
	pj_bool_t           active_;
	mutex               terminations_lock_;
	termination_map_t   terminations_;
	mutex               rooms_lock_;
	room_map_t          rooms_;
	pjmedia_rtp_session rtp_in_session_;
	pjmedia_rtp_session rtp_out_session_;
	mutex               rtp_out_sess_lock_;
	PoolThread<std::function<void()>> sync_thread_pool_;
	PoolThread<std::function<void()>> async_thread_pool_;

private:
	typedef std::pair< pj_uint32_t, pj_uint16_t>	mtu32wu16;
	typedef std::map< mtu32wu16, pj_uint64_t >		mtu32wu16tou64;
	typedef std::pair< mtu32wu16, pj_uint64_t>		mtu32wu16wu64;
	typedef mtu32wu16tou64::iterator				mtu32wu16tou64Itr;
	typedef map< pj_uint32_t, mtu32wu16tou64 >		mtu32toMapu32wu16tou64;
	typedef mtu32toMapu32wu16tou64::iterator		mtu32toMapu32wu16tou64Itr;
	mutex							moAvsStatusInfo_lock_;
	mtu32toMapu32wu16tou64			moAvsStatusInfo;
	CMongoClientBase				moMomgoDBClient;
	
	typedef std::set<pj_uint32_t>					mtu32set;
	typedef mtu32set::iterator						mtu32setItr;
	typedef std::pair<pj_uint32_t, mtu32set>			mtu32wu32List;
	typedef std::map< pj_uint32_t, mtu32set>			mtu32tou32List;
	typedef mtu32tou32List::iterator				mtu32tou32ListItr;
	mutex							moClientidtoRoomList_lock_;
	mtu32tou32List					moClientidtoRoomList;
	
	typedef std::pair<pj_uint32_t, pj_uint32_t>		mtu32tou32pair;
	typedef std::map<pj_uint32_t, pj_uint32_t>		mtu32tou32map;
	typedef mtu32tou32map::iterator					mtu32tou32mapItr;
	typedef std::pair< pj_uint32_t, mtu32tou32map>		mtu32wu32tou32pair;
	typedef std::map< pj_uint32_t, mtu32tou32map>		mtu32wu32tou32map;
	typedef mtu32wu32tou32map::iterator				mtu32wu32tou32mapItr;

	mutex							moRoomIDtoClientidwfdlist_lock_;
	mtu32wu32tou32map					moRoomIDtoClientidwfdlist;
};

#endif
