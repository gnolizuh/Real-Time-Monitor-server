#ifndef __AVS_PROXY_RES_LOGIN_SCENE__
#define __AVS_PROXY_RES_LOGIN_SCENE__

#include "Parameter.h"
#include "Scene.h"

class ResLoginParameter
	: public UdpParameter
{
public:
	ResLoginParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint8_t response_code_;
};

class ResLoginScene
	: public UdpScene
{
public:
	ResLoginScene() {}
	virtual ~ResLoginScene() {}

	virtual void Maintain(UdpParameter *, RoomMgr *);
};

#endif
