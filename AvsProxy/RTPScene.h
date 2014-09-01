#ifndef __AVS_PROXY_RTP_SCENE__
#define __AVS_PROXY_RTP_SCENE__

#include <vector>
#include "Parameter.h"
#include "Scene.h"

using std::vector;

class RTPParameter
	: public UdpParameter
{
public:
	RTPParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int64_t  user_id_;
	pj_uint32_t ssrc_;
	pj_uint8_t  media_type_;
	vector<pj_uint8_t> rtp_package_;
};

class RTPScene
	: public UdpScene
{
public:
	RTPScene() {}
	virtual ~RTPScene() {}

	virtual scene_opt_t Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer);
};

#endif
