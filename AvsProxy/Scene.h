#ifndef __AVS_PROXY_SCENE__
#define __AVS_PROXY_SCENE__

#include "Com.h"
#include "Room.h"
#include "Parameter.h"
#include "Termination.h"

class RoomMgr;

class TcpScene
{
public:
	TcpScene() {}
	virtual ~TcpScene() {}

	virtual void Maintain(TcpParameter *, Termination *, RoomMgr *) {}
};

class UdpScene
{
public:
	UdpScene() {}
	virtual ~UdpScene() {}

	virtual void Maintain(UdpParameter *, RoomMgr *) {}
};

#endif
