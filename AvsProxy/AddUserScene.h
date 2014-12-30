#ifndef __AVS_PROXY_ADD_USER_SCENE__
#define __AVS_PROXY_ADD_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"

class AddUserParameter
	: public UdpParameter
{
public:
	AddUserParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int64_t  user_id_;
	pj_uint32_t mic_id_;
};

class AddUserScene
	: public UdpScene
{
public:
	AddUserScene() {}
	virtual ~AddUserScene() {}

	virtual scene_opt_t Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer);
};

#endif
