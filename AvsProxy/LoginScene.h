#ifndef __AVS_PROXY_LOGIN_SCENE__
#define __AVS_PROXY_LOGIN_SCENE__

#include "Parameter.h"
#include "Scene.h"

class LoginParameter
	: public Parameter
{
public:
	LoginParameter(const pj_uint8_t *);

	pj_int32_t media_port_;
};

class LoginScene
	: public Scene
{
public:
	LoginScene() {}
	virtual ~LoginScene() {}

	virtual void Maintain(Parameter *, Termination *, Room *);
};

#endif
