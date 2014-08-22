#ifndef __AVS_PROXY_LOGIN_SCENE__
#define __AVS_PROXY_LOGIN_SCENE__

#include "Parameter.h"
#include "Scene.h"
#include "Config.h"

class LoginParameter
	: public TcpParameter
{
public:
	LoginParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint16_t media_port_;
};

class LoginScene
	: public TcpScene
{
public:
	LoginScene() {}
	virtual ~LoginScene() {}

	virtual scene_opt_t Maintain(TcpParameter *, Termination *);
};

#endif
