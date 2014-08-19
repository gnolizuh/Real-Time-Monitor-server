#ifndef __AVS_PROXY_MOD_USER_MEDIA_SCENE__
#define __AVS_PROXY_MOD_USER_MEDIA_SCENE__

#include "Parameter.h"
#include "Scene.h"

class ModUserMediaParameter
	: public UdpParameter
{
public:
	ModUserMediaParameter(const pj_uint8_t *, pj_uint16_t);

	pj_int64_t  user_id_;
	pj_uint32_t audio_ssrc_;
	pj_uint32_t video_ssrc_;
};

class ModUserMediaScene
	: public UdpScene
{
public:
	ModUserMediaScene() {}
	virtual ~ModUserMediaScene() {}

	virtual scene_opt_t Maintain(UdpParameter *, Room *, pj_buffer_t &);
};

#endif
