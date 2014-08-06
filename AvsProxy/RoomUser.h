#ifndef __AVS_PROXY_ROOM_USER__
#define __AVS_PROXY_ROOM_USER__

#include <list>
#include "Termination.h"

using std::list;

class RoomUser
{
public:
	pj_int64_t           user_id_;
	pj_uint32_t          audio_ssrc_;
	pj_uint32_t          video_ssrc_;
	list<Termination *>  members_;         // follow此房间的客户端列表
};

#endif
