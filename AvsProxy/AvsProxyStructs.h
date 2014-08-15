#ifndef __AVS_PROXY_AVS_PROXY_STRUCTS__
#define __AVS_PROXY_AVS_PROXY_STRUCTS__

#include "Com.h"

#pragma pack(1)
typedef struct
{
	void Serialize()
	{
		proxy_request_type = serialize(proxy_request_type);
		proxy_id = serialize(proxy_id);
		room_id = serialize(room_id);
	}

	pj_uint16_t proxy_request_type;
	pj_uint16_t proxy_id;
	pj_int32_t  room_id;
} request_to_avs_login_t;

typedef struct 
{
	void Serialize()
	{
		proxy_request_type = serialize(proxy_request_type);
		proxy_id = serialize(proxy_id);
		room_id = serialize(room_id);
	}

	pj_uint16_t proxy_request_type;
	pj_uint16_t proxy_id;
	pj_int32_t  room_id;
} request_to_avs_logout_t;

typedef struct 
{
	void Serialize()
	{
		proxy_request_type = serialize(proxy_request_type);
		proxy_id = serialize(proxy_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
		link_media_mask = serialize(link_media_mask);
	}

	pj_uint16_t proxy_request_type;
	pj_uint16_t proxy_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
	pj_uint8_t  link_media_mask;
} request_to_avs_link_user_t;

typedef struct 
{
	void Serialize()
	{
		proxy_request_type = serialize(proxy_request_type);
		proxy_id = serialize(proxy_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
		unlink_media_mask = serialize(unlink_media_mask);
	}

	pj_uint16_t proxy_request_type;
	pj_uint16_t proxy_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
	pj_uint8_t  unlink_media_mask;
} request_to_avs_unlink_user_t;

typedef struct 
{
	void Serialize()
	{
		proxy_request_type = serialize(proxy_request_type);
		proxy_id = serialize(proxy_id);
		room_id = serialize(room_id);
	}

	pj_uint16_t proxy_request_type;
	pj_uint16_t proxy_id;
	pj_int32_t  room_id;
} request_to_avs_keep_alive_t;

#pragma pack()

#endif
