#ifndef __AVS_PROXY_AVS_PROXY_STRUCTS__
#define __AVS_PROXY_AVS_PROXY_STRUCTS__

#include "Com.h"

template<typename Type>
inline Type serialize(Type t)
{
	if ( sizeof(t) == sizeof(pj_uint8_t) )
	{
		return t;
	}
	else if ( sizeof(t) == sizeof(pj_uint16_t) )
	{
		return (Type)pj_htons((pj_uint16_t)t);
	}
	else if ( sizeof(t) == sizeof(pj_uint32_t) )
	{
		return (Type)pj_htonl((pj_uint32_t)t);
	}
	else if ( sizeof(t) == sizeof(pj_uint64_t) )
	{
		return (Type)pj_htonll((pj_uint64_t)t);
	}
	else
	{
		pj_assert(!"Don't serialize a number which value is more then 64bit!!");
		return (Type)0;
	}
}

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
} request_login_t;

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
} request_logout_t;

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
} request_link_user_t;

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
} request_unlink_user_t;

#endif
