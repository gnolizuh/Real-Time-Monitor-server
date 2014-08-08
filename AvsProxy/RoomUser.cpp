#include "RoomUser.h"

SafeUdpSocket g_safe_udp_sock;

pj_status_t SafeUdpSocket::Open()
{
	return pj_open_udp_transport(NULL, 0, udp_sock_);
}

pj_status_t SafeUdpSocket::Close()
{
	std::lock_guard<std::mutex> lock( protection_ );
	return pj_sock_close(udp_sock_);
}

pj_status_t SafeUdpSocket::Sendto(const void *buf,
								 pj_ssize_t *buflen,
								 const pj_str_t &toip,
								 pj_uint16_t toport)
{
	pj_sockaddr_in addr;
	pj_status_t status;
	status = pj_sockaddr_in_init(&addr, &toip, toport);
	RETURN_VAL_IF_FAIL( status == PJ_SUCCESS, status );

	std::lock_guard<std::mutex> lock( protection_ );
	return pj_sock_sendto(udp_sock_, buf, buflen, 0, &addr, sizeof(addr));
}

RoomUser::RoomUser()
	: follows_()
{
}

pj_status_t RoomUser::AddFollower(pj_int64_t follower_id, pj_str_t ip, pj_int32_t port, pj_uint8_t media_mask)
{
	// Lock section.
	followers_map_t::iterator pfollower = follows_.find(follower_id);
	if ( pfollower == follows_.end() )
	{
		followers_map_t::mapped_type follow = new follower_t();
		follow->ip = pj_str(ip.ptr);
		follow->port = port;
		follow->media_mask = media_mask;
		follows_[follower_id] = follow;

		return PJ_SUCCESS;
	}

	return PJ_EEXISTS;
}

void RoomUser::OnRxAudio(const vector<uint8_t> &audio_package)
{
	// Lock section.
	for( auto follow : follows_ )
	{
		followers_map_t::key_type follower_id = follow.first;
		followers_map_t::mapped_type follower = follow.second;
		if(follower->media_mask & MEDIA_MASK_AUDIO)
		{
			pj_ssize_t len = audio_package.size();
			g_safe_udp_sock.Sendto(&audio_package, &len, follower->ip, follower->port);
		}
	}
}

void RoomUser::OnRxVideo(const vector<uint8_t> &video_package)
{
	for( auto follow : follows_ )
	{
		followers_map_t::key_type follower_id = follow.first;
		followers_map_t::mapped_type follower = follow.second;
		if(follower->media_mask & MEDIA_MASK_VIDEO)
		{
			pj_ssize_t len = video_package.size();
			g_safe_udp_sock.Sendto(&video_package, &len, follower->ip, follower->port);
		}
	}
}
