#ifndef __AVS_PROXY_ROOM_USER__
#define __AVS_PROXY_ROOM_USER__

#include <list>
#include "Termination.h"

using std::list;

class RoomUser
{
	pj_int64_t           user_id;
	pj_uint32_t          audio_ssrc;
	pj_uint32_t          video_ssrc;
	list<Termination *>  members_;         // follow此房间的客户端列表
};

#endif
