#include "Room.h"

pj_status_t Room::Link()
{
	return PJ_SUCCESS;
}

pj_status_t Room::Unlink()
{
	return PJ_SUCCESS;
}

RoomUser *Room::GetUser(pj_int64_t user_id)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	return puser->second;
}