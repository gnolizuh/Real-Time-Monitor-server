#pragma once

#include "Parameter.h"
#include "Scene.h"
class RoomMgr;
class LinkRoomParameter
	: public TcpParameter
{
public:
	LinkRoomParameter(const pj_uint8_t *, pj_uint16_t);
	pj_uint32_t room_id;
};

class CLinkRoomScene
	: public TcpScene
{
public:
	CLinkRoomScene();
	virtual ~CLinkRoomScene();

	virtual scene_opt_t Maintain(shared_ptr<TcpParameter> ptr_tcp_param, RoomMgr*,Termination *termination);
};

