#ifndef __AVS_PROXY_DISCONNECT_SCENE__
#define __AVS_PROXY_DISCONNECT_SCENE__

#include "Parameter.h"
#include "Scene.h"
class RoomMgr;
class DisconnectScene
{
public:
	scene_opt_t Maintain(Termination *termination, RoomMgr *room_mgr);
};

#endif