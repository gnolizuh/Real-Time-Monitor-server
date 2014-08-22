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
		length = serialize((pj_uint16_t)(Size() - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_count = serialize(room_count);
		for(unsigned i = 0; i < rooms.size(); ++ i)
		{
			rooms[i].room_id = serialize(rooms[i].room_id);
			rooms[i].user_count = serialize(rooms[i].user_count);
			for(unsigned j = 0; j < rooms[i].users.size(); ++ j)
			{
				rooms[i].users[j].user_id = serialize(rooms[i].users[j].user_id);
				rooms[i].users[j].audio_ssrc = serialize(rooms[i].users[j].audio_ssrc);
				rooms[i].users[j].video_ssrc = serialize(rooms[i].users[j].video_ssrc);
			}
		}
	}

	pj_ssize_t Size()
	{
		pj_ssize_t size = 12;
		for(unsigned i = 0; i < rooms.size(); ++ i)
		{
			size += sizeof(pj_int32_t) + sizeof(pj_uint32_t);
			for(unsigned j = 0; j < rooms.size(); ++ j)
			{
				size += sizeof(pj_int64_t) + sizeof(pj_uint32_t) + sizeof(pj_uint32_t);
			}
		}
		return size; 
	}

	pj_status_t Copy(pj_uint8_t *dst, pj_uint32_t len)
	{
		pj_uint32_t pos = 0;
		pj_memcpy(dst + pos, this, 12); pos += 12;
		RETURN_VAL_IF_FAIL(pos <= len, PJ_EINVAL);

		for(unsigned i = 0; i < rooms.size(); ++ i)
		{
			pj_memcpy(dst + pos, &rooms[i], 8); pos += 8;
			RETURN_VAL_IF_FAIL(pos <= len, PJ_EINVAL);			

			for(unsigned j = 0; j < rooms[i].users.size(); ++ j)
			{
				pj_memcpy(dst + pos, &rooms[i].users[j], sizeof(user_info_t)); pos += sizeof(user_info_t);
				RETURN_VAL_IF_FAIL(pos <= len, PJ_EINVAL);
			}
		}

		return PJ_SUCCESS;
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
		length = serialize((pj_uint16_t)(sizeof(request_to_client_room_mod_media_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
		audio_ssrc = serialize(audio_ssrc);
		video_ssrc = serialize(video_ssrc);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
	pj_uint32_t audio_ssrc;
	pj_uint32_t video_ssrc;
} request_to_client_room_mod_media_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_client_room_add_user_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
} request_to_client_room_add_user_t;

typedef struct
{
	void Serialize()
	{
		length = serialize((pj_uint16_t)(sizeof(request_to_client_room_del_user_t) - sizeof(length)));
		client_request_type = serialize(client_request_type);
		proxy_id = serialize(proxy_id);
		client_id = serialize(client_id);
		room_id = serialize(room_id);
		user_id = serialize(user_id);
	}

private:
	pj_uint16_t length;

public:
	pj_uint16_t client_request_type;
	pj_uint16_t proxy_id;
	pj_uint16_t client_id;
	pj_int32_t  room_id;
	pj_int64_t  user_id;
} request_to_client_room_del_user_t;

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
