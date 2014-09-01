#ifndef __AVS_PROXY_KEEP_ALIVE_SCENE__
#define __AVS_PROXY_KEEP_ALIVE_SCENE__

#include "Parameter.h"
#include "Scene.h"

class KeepAliveParameter
	: public TcpParameter
{
public:
	KeepAliveParameter(const pj_uint8_t *, pj_uint16_t);
};

class KeepAliveScene
	: public TcpScene
{
public:
	KeepAliveScene() {}
	virtual ~KeepAliveScene() {}

	virtual scene_opt_t Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Termination *termination, pj_buffer_t &buffer);
};

#endif