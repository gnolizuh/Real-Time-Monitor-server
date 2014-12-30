#ifndef __RECV_AVS_STATUS_INFO__
#define __RECV_AVS_STATUS_INFO__

#include "Parameter.h"
#include "Scene.h"
class RoomMgr;
class CRecvAVSStatusParameter
	:public UdpParameter
{
public:
	CRecvAVSStatusParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint32_t	mu32avs_id;
	pj_uint32_t mu32avs_ip;
	pj_uint16_t mu16avs_port;
};

class CRecvAVSStatusInfo
	:public UdpScene
{
public:
	CRecvAVSStatusInfo ();
	virtual ~CRecvAVSStatusInfo ();
	scene_opt_t Maintain(shared_ptr<UdpParameter>, RoomMgr *);
};

#endif

