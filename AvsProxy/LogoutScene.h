#ifndef __AVS_PROXY_LOGOUT_SCENE__
#define __AVS_PROXY_LOGOUT_SCENE__

#include "Parameter.h"
#include "Scene.h"

class LogoutParameter
	: public Parameter
{
public:
	LogoutParameter(const pj_uint8_t *);
};

class LogoutScene
	: public Scene
{
public:
	LogoutScene() {}
	virtual ~LogoutScene() {}

	virtual void Maintain(Parameter *, Termination *, Room *);
};

#endif
