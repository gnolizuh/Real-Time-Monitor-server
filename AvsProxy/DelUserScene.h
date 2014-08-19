#ifndef __AVS_PROXY_DEL_USER_SCENE__
#define __AVS_PROXY_DEL_USER_SCENE__

#include "Parameter.h"
#include "Scene.h"

class DelUserParameter
	: public UdpParameter
{
public:
	DelUserParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int64_t  user_id_;
};

class DelUserScene
	: public UdpScene
{
public:
	DelUserScene() {}
	virtual ~DelUserScene() {}

	virtual scene_opt_t Maintain(UdpParameter *, Room *, pj_buffer_t &);
};

#endif
