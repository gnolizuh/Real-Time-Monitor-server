#ifndef __AVS_PROXY_CLIENT_STRUCTS__
#define __AVS_PROXY_CLIENT_STRUCTS__

#include <vector>
#include "Com.h"

using std::vector;

#pragma pack(1)
typedef struct
{
	pj_int64_t  user_id;
	pj_uint32_t audio_ssrc;
	pj_uint32_t video_ssrc;
} user_info_t;

typedef struct
{
	pj_int32_t  room_id;
	pj_uint32_t user_count;
	vector<user_info_t> users;
} room_info_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_client_rooms_info_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_count = serialize(room_count);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_uint32_t room_count;
	vector<room_info_t> rooms;
} request_to_client_rooms_info_t;

typedef struct 
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_client_force_logout_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
} request_to_client_force_logout_t;

typedef struct 
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(response_to_client_keep_alive_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
} response_to_client_keep_alive_t;
#pragma pack()

#endif
