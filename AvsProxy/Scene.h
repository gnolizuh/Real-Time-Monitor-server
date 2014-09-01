#ifndef __AVS_PROXY_SCENE__
#define __AVS_PROXY_SCENE__

#include "Com.h"
#include "Parameter.h"
#include "Termination.h"
#include "Room.h"
#include "AvsProxyStructs.h"

using std::shared_ptr;

class TcpScene
{
public:
	TcpScene() {}
	virtual ~TcpScene() {}

	virtual scene_opt_t Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Termination *termination, pj_buffer_t &buffer) { return SCENE_OPT_NONE; }
};

class UdpScene
{
public:
	UdpScene() {}
	virtual ~UdpScene() {}

	virtual scene_opt_t Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer) { return SCENE_OPT_NONE; }
};

#endif
