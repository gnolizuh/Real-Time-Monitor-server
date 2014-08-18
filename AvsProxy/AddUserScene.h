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
};

class AddUserScene
	: public UdpScene
{
public:
	AddUserScene() {}
	virtual ~AddUserScene() {}

	virtual void Maintain(UdpParameter *, RoomMgr *);
};

#endif
