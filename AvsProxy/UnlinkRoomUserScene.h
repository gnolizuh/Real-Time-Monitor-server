#ifndef __AVS_PROXY_UNLINK_ROOM_USER_SCENE__
#define __AVS_PROXY_UNLINK_ROOM_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"

class UnlinkRoomUserParameter
	: public Parameter
{
public:
	UnlinkRoomUserParameter(const pj_uint8_t *);

	pj_int32_t room_id_;
	pj_int64_t user_id_;
	pj_uint8_t unlink_media_mask_;
};

class UnlinkRoomUserScene
	: public Scene
{
public:
	UnlinkRoomUserScene() {}
	virtual ~UnlinkRoomUserScene() {}

	virtual void Maintain(Parameter *, Termination *, Room *);
};

#endif