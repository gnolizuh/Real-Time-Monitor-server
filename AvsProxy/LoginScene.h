#ifndef __AVS_PROXY_LOGIN_SCENE__
#define __AVS_PROXY_LOGIN_SCENE__

#include "Parameter.h"
#include "Scene.h"

class LoginParameter
	: public TcpParameter
{
public:
	LoginParameter(const pj_uint8_t *, pj_uint16_t);
};

class LoginScene
	: public TcpScene
{
public:
	LoginScene() {}
	virtual ~LoginScene() {}

	virtual void Maintain(TcpParameter *, Termination *, Room *);
};

#endif
