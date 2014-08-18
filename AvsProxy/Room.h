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
	pj_status_t OnLinkUser(pj_int64_t, pj_uint16_t, const pj_str_t &, pj_int32_t, pj_uint8_t);
	pj_status_t OnUnlinkUser(pj_int64_t, pj_uint16_t);
	pj_status_t OnAddUser(pj_int64_t, pj_uint32_t, pj_uint32_t);
	pj_status_t OnAddUser(pj_int64_t);
	pj_status_t OnModUser(pj_int64_t, pj_uint32_t, pj_uint32_t);
	pj_status_t OnDelUser(pj_int64_t);
	pj_status_t OnLogin(pj_bool_t);
	void        OnRxRtp(pj_int64_t, pj_uint8_t, const vector<pj_uint8_t> &);
	RoomUser *  GetUser(pj_int64_t);

private:
	mutex      room_lock_;
	room_id_t  room_id_;
	user_map_t online_users_;    // 此房间的所有上麦用户信息
};

#endif
