#ifndef __AVS_PROXY_LINK_ROOM_USER_SCENE__
#define __AVS_PROXY_LINK_ROOM_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"
#include "RoomUser.h"

class LinkRoomUserParameter
	: public TcpParameter
{
public:
	LinkRoomUserParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int32_t room_id_;
	pj_int64_t user_id_;
	pj_uint8_t link_media_mask_;
};

class LinkRoomUserScene
	: public TcpScene
{
public:
	LinkRoomUserScene() {}
	virtual ~LinkRoomUserScene() {}

	virtual void Maintain(TcpParameter *, Termination *, RoomMgr *);
};

#endif