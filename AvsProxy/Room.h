#ifndef __AVS_PROXY_ROOM__
#define __AVS_PROXY_ROOM__

#include <vector>
#include <mutex>
#include <map>

#include "Com.h"
#include "RoomUser.h"
#include "Termination.h"

using std::map;
using std::vector;
using std::mutex;
using std::lock_guard;

typedef map<pj_int64_t, RoomUser *> user_map_t;
class Room /* same as AVS */
{
public:
	Room(const pj_str_t &, pj_uint16_t, pj_sock_t &, pjmedia_rtp_session &, mutex &);
	~Room();

	pj_status_t Prepare();
	pj_status_t Launch();
	void        Destory();
	user_map_t &GetUsers();
	pj_status_t OnLinkUser(pj_int64_t, pj_uint16_t, const pj_str_t &, pj_uint16_t, pj_uint8_t, pj_bool_t &);
	pj_status_t OnUnlinkUser(pj_int64_t, pj_uint16_t, pj_bool_t &);
	pj_status_t OnAddUser(pj_int64_t, pj_uint32_t, pj_uint32_t);
	pj_status_t OnAddUser(pj_int64_t);
	pj_status_t OnModUser(pj_int64_t, pj_uint32_t, pj_uint32_t);
	pj_status_t OnDelUser(pj_int64_t);
	pj_status_t OnRxLogin(pj_bool_t);
	pj_status_t OnRxKeepAlive();
	void        OnRxRtp(pj_int64_t, pj_uint8_t, const vector<pj_uint8_t> &);
	RoomUser *  GetUser(pj_int64_t);
	pj_status_t SendRTPPacket(const void *, pj_ssize_t *);

private:
	pj_str_t    room_ip_;
	pj_uint16_t room_port_;
	mutex       room_lock_;
	room_id_t   room_id_;
	user_map_t  online_users_;    // 此房间的所有上麦用户信息
	pj_sock_t           &ref_udp_sock_;
	pjmedia_rtp_session &ref_rtp_session_;
	mutex               &ref_rtp_sess_lock_;
};

#endif
