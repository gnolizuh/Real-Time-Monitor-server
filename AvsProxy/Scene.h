#ifndef __AVS_PROXY_SCENE__
#define __AVS_PROXY_SCENE__

#include "Com.h"
#include "Parameter.h"
#include "Termination.h"
#include "Room.h"
#include "AvsProxyStructs.h"

class TcpScene
{
public:
	TcpScene() {}
	virtual ~TcpScene() {}

	virtual scene_opt_t Maintain(TcpParameter *, Termination *, pj_buffer_t &) { return SCENE_OPT_NONE; }
};

class UdpScene
{
public:
	UdpScene() {}
	virtual ~UdpScene() {}

	virtual scene_opt_t Maintain(UdpParameter *, Room *, pj_buffer_t &) { return SCENE_OPT_NONE; }
};

#endif
