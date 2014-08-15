#ifndef __AVS_PROXY_CLIENT_STRUCTS__
#define __AVS_PROXY_CLIENT_STRUCTS__

#include "Com.h"

#pragma pack(1)
typedef struct 
{
	void Serialize()
	{
		length = serialize(length);
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_count = serialize(room_count);
	}

	pj_uint16_t length;
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_uint32_t room_count;
} request_to_client_rooms_info_t;

typedef struct 
{
	void Serialize()
	{
		length = serialize(length);
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

	pj_uint16_t length;
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
} request_to_client_force_logout_t;

typedef struct 
{
	void Serialize()
	{
		length = serialize(length);
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

	pj_uint16_t length;
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
} response_to_client_keep_alive_t;
#pragma pack()

#endif
