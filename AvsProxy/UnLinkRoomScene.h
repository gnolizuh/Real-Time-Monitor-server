#pragma once

#include "Parameter.h"
#include "Scene.h"
class RoomMgr;
class UnLinkRoomParameter
	:public TcpParameter
{
public:
	UnLinkRoomParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint32_t room_id;
	pj_uint16_t user_id;

};
class CUnLinkRoomScene
	:public TcpScene
{
public:

	CUnLinkRoomScene()
	{
	}

	virtual ~CUnLinkRoomScene()
	{
	}

	scene_opt_t Maintain(shared_ptr<TcpParameter> ptr_tcp_param, RoomMgr*, Termination *termination);
};

