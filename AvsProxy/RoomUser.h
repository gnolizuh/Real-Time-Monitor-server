#ifndef __AVS_PROXY_ROOM_USER__
#define __AVS_PROXY_ROOM_USER__

#include <vector>
#include <map>
#include <mutex>

#include "Termination.h"

using std::map;
using std::vector;
using std::mutex;

typedef struct
{
	pj_str_t   ip;
	pj_int32_t port;
	pj_uint8_t media_mask;
} follower_t;

typedef map<pj_uint16_t, follower_t *> followers_map_t;

class SafeUdpSocket
{
public:
	pj_status_t Open();
	pj_status_t Close();
	pj_status_t Sendto(const void *, pj_ssize_t *, const pj_str_t &, pj_uint16_t);

private:
	pj_sock_t udp_sock_;
	mutex protection_;
};

class RoomUser
{
public:
	RoomUser();

	pj_status_t OnLinkRoomUser(pj_uint16_t, const pj_str_t &, pj_int32_t, pj_uint8_t);
	pj_status_t OnUnlinkRoomUser(pj_uint16_t);
	void        OnRxAudio(const vector<uint8_t> &);
	void        OnRxVideo(const vector<uint8_t> &);

	pj_int64_t      user_id_;
	pj_uint32_t     audio_ssrc_;
	pj_uint32_t     video_ssrc_;
	followers_map_t follows_;         // follow此房间的客户端列表
};

#endif
