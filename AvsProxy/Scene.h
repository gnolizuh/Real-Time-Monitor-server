#ifndef __AVS_PROXY_SCENE__
#define __AVS_PROXY_SCENE__

#include "Com.h"
#include "Room.h"
#include "Parameter.h"
#include "Termination.h"

class TcpScene
{
public:
	TcpScene() {}
	virtual ~TcpScene() {}

	virtual void Maintain(TcpParameter *, Termination *, Room *) = 0;
};

class UdpScene
{
public:
	UdpScene() {}
	virtual ~UdpScene() {}

	virtual void Maintain(UdpParameter *, Room *) = 0;
};

#endif
