#ifndef __AVS_PROXY_USERS_INFO_SCENE__
#define __AVS_PROXY_USERS_INFO_SCENE__

#include <vector>
#include "Parameter.h"
#include "Scene.h"

using std::vector;

typedef struct
{
	pj_int64_t  user_id;
	pj_uint32_t audio_ssrc;
	pj_uint32_t video_ssrc;
} user_t;

class UsersInfoParameter
	: public UdpParameter
{
public:
	UsersInfoParameter(const pj_uint8_t *, pj_uint16_t);

	pj_uint8_t     user_count_;
	vector<user_t> users_;
};

class UsersInfoScene
	: public UdpScene
{
public:
	UsersInfoScene() {}
	virtual ~UsersInfoScene() {}

	virtual void Maintain(UdpParameter *, Room *);
};

#endif
