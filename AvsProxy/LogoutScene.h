#ifndef __AVS_PROXY_LOGOUT_SCENE__
#define __AVS_PROXY_LOGOUT_SCENE__

#include "Parameter.h"
#include "Scene.h"

class LogoutParameter
	: public TcpParameter
{
public:
	LogoutParameter(const pj_uint8_t *, pj_uint16_t);
};

class LogoutScene
	: public TcpScene
{
public:
	LogoutScene() {}
	virtual ~LogoutScene() {}

	virtual void Maintain(TcpParameter *, Termination *, Room *);
};

#endif
