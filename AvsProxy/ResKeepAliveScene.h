#ifndef __AVS_PROXY_RES_KEEP_ALIVE_SCENE__
#define __AVS_PROXY_RES_KEEP_ALIVE_SCENE__

#include "Parameter.h"
#include "Scene.h"

class ResKeepAliveParameter
	: public UdpParameter
{
public:
	ResKeepAliveParameter(const pj_uint8_t *, pj_uint16_t);
};

class ResKeepAliveScene
	: public UdpScene
{
public:
	ResKeepAliveScene() {}
	virtual ~ResKeepAliveScene() {}

	virtual void Maintain(UdpParameter *, RoomMgr *);
};

#endif
