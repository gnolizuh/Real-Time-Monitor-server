#ifndef __AVS_PROXY_ROOM__
#define __AVS_PROXY_ROOM__

#include <list>
#include <map>

#include "Com.h"
#include "RoomUser.h"
#include "Termination.h"

using std::list;
using std::map;

typedef map<pj_int64_t, RoomUser *> user_map_t;
class Room/* same as AVS */
{
public:
	pj_status_t Link();
	pj_status_t Unlink();

	pj_status_t Add(pj_int64_t, pj_uint32_t, pj_uint32_t);
	pj_status_t Add(pj_int64_t);
	pj_status_t Mod(pj_int64_t, pj_uint32_t, pj_uint32_t);
	pj_status_t Del(pj_int64_t);

	RoomUser *GetUser(pj_int64_t);

private:
	room_id_t       room_id_;
	user_map_t      online_users_;    // 此房间的所有上麦用户信息
	// list<Address *> room_addr_;
};

#endif
