#pragma once
#include "Parameter.h"
#include "Scene.h"
class RoomMgr;
class NATforClientParam
	: public UdpParameter
{
public:
	NATforClientParam(const pj_uint8_t*, pj_uint16_t);
	pj_uint16_t	client_id;
};

class CNATforClient
	: public UdpScene
{
public:
	CNATforClient();
	virtual ~CNATforClient();

	virtual scene_opt_t Maintain(shared_ptr<UdpParameter>, RoomMgr *, pj_sockaddr_in&);
};

