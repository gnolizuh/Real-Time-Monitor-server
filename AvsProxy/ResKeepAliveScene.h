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

	virtual scene_opt_t Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer);
};

#endif
