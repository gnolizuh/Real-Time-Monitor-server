#ifndef __AVS_PROXY_UNLINK_ROOM_USER_SCENE__
#define __AVS_PROXY_UNLINK_ROOM_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"

class UnlinkRoomUserParameter
	: public TcpParameter
{
public:
	UnlinkRoomUserParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int32_t room_id_;
	pj_int64_t user_id_;
	pj_uint8_t unlink_media_mask_;
};

class UnlinkRoomUserScene
	: public TcpScene
{
public:
	UnlinkRoomUserScene() {}
	virtual ~UnlinkRoomUserScene() {}

	virtual scene_opt_t Maintain(TcpParameter *, Room *, Termination *, pj_buffer_t &);
};

#endif