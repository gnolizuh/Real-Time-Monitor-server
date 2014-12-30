#include "RoomUser.h"

SafeUdpSocket g_safe_client_sock;

pj_status_t SafeUdpSocket::Open(pj_str_t *ip, pj_uint16_t port, pj_sock_t &sock)
{
	return pj_open_udp_transport(ip, port, udp_sock_);
}

pj_status_t SafeUdpSocket::Open()
{
	return pj_open_udp_transport(NULL, 0, udp_sock_);
}

void SafeUdpSocket::Open(pj_sock_t sock)
{
	udp_sock_ = sock;
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

RoomUser::RoomUser(pj_int64_t userid, pj_uint32_t mic_id, pj_uint32_t vssrc, pj_uint32_t assrc)
	: user_id_(userid)
	, mic_id_(mic_id)
	, audio_ssrc_(assrc)
	, video_ssrc_(vssrc)
	, follows_()
{
}

void RoomUser::Destory()
{
	lock_guard<mutex> lock(user_lock_);

	followers_map_t::iterator pfollower = follows_.begin();
	for(; pfollower != follows_.end(); )
	{
		followers_map_t::mapped_type follow = pfollower->second;
		if(follow != nullptr)
		{
			delete follow;
			follow = nullptr;
		}

		pfollower = follows_.erase(pfollower);
	}
}

void RoomUser::OnConsole(stringstream &output)
{
	char printer[128] = {0};
	snprintf(printer, sizeof(printer) - 1, "\t\tOnline user id: %ld audio_ssrc: %u video_ssrc: %u\n",
		user_id_, audio_ssrc_, video_ssrc_);
	output << printer;

	followers_map_t::iterator pfollow = follows_.begin();
	for(; pfollow != follows_.end(); ++ pfollow)
	{
		followers_map_t::key_type client_id = pfollow->first;
		followers_map_t::mapped_type follower = pfollow->second;
		if(follower != nullptr)
		{
			snprintf(printer, sizeof(printer) - 1, "\t\t\tFollower id: %u ip:%s port:%d mask:%x\n",
				client_id, GET_STRING(follower->ip.ptr), follower->port, follower->media_mask);
			output << printer;
		}
	}
}

void RoomUser::SetCurUserID(pj_int64_t user_id)
{
	lock_guard<mutex> lock(user_lock_);
	user_id_ = user_id;
}

pj_int64_t RoomUser::GetCurUserID()
{
	lock_guard<mutex> lock(user_lock_);
	return user_id_;
}

void RoomUser::SetAVSSrc(pj_uint32_t assrc, pj_uint32_t vssrc)
{
	lock_guard<mutex> lock(user_lock_);
	audio_ssrc_ = assrc;
	video_ssrc_ = vssrc;
}

pj_uint32_t RoomUser::GetASSrc()
{
	lock_guard<mutex> lock(user_lock_);
	return audio_ssrc_;
}

pj_uint32_t RoomUser::GetVSSrc()
{
	lock_guard<mutex> lock(user_lock_);
	return video_ssrc_;
}

void RoomUser::GetCurUserInfo(pj_int64_t& userid, pj_uint32_t& mic_id, pj_uint32_t& assrc, pj_uint32_t& vssrc)
{
	lock_guard<mutex> lock(user_lock_);
	userid = user_id_;
	mic_id = mic_id_;
	assrc = audio_ssrc_;
	vssrc = video_ssrc_;
}

pj_status_t RoomUser::OnLink(pj_uint16_t client_id,
							 const pj_str_t &ip,
							 pj_uint16_t port,
							 pj_uint8_t media_mask,
							 pj_bool_t &is_continue)
{
	lock_guard<mutex> lock(user_lock_);

	followers_map_t::iterator pfollower = follows_.find(client_id);
	RETURN_VAL_IF_FAIL(pfollower == follows_.end(), PJ_EEXISTS);

	if(follows_.empty())
	{
		is_continue = PJ_TRUE;
	}

	followers_map_t::mapped_type follow = new follower_t();
	follow->ip = pj_str(ip.ptr);
	follow->port = port;
	follow->media_mask = media_mask;
	follows_[client_id] = follow;

	return PJ_SUCCESS;
}

pj_status_t RoomUser::OnUnlink(pj_uint16_t client_id, pj_bool_t &is_continue)
{
	lock_guard<mutex> lock(user_lock_);

	followers_map_t::iterator pfollower = follows_.find(client_id);
	RETURN_VAL_IF_FAIL(pfollower != follows_.end(), PJ_ENOTFOUND);

	followers_map_t::mapped_type follow = pfollower->second;
	RETURN_VAL_IF_FAIL(follow != nullptr, PJ_ENOTFOUND);

	follows_.erase(pfollower);
	delete follow;
	follow = nullptr;

	if(follows_.empty())
	{
		is_continue = PJ_TRUE;
	}

	return PJ_SUCCESS;
}

void RoomUser::OnRxAudio(const vector<uint8_t> &audio_package)
{
	lock_guard<mutex> lock(user_lock_);
	for( followers_map_t::iterator pfollow = follows_.begin();
		pfollow != follows_.end();
		++ pfollow)
	{
		followers_map_t::key_type follower_id = pfollow->first;
		followers_map_t::mapped_type follower = pfollow->second;
		if(follower->media_mask & MEDIA_MASK_AUDIO)
		{
			pj_ssize_t len = audio_package.size();
			g_safe_client_sock.Sendto(&audio_package, &len, follower->ip, follower->port);
		}
	}
}

void RoomUser::OnRxVideo(const vector<uint8_t> &video_package)
{
	lock_guard<mutex> lock(user_lock_);
	for( followers_map_t::iterator pfollow = follows_.begin();
		pfollow != follows_.end();
		++ pfollow)
	{
		followers_map_t::key_type follower_id = pfollow->first;
		followers_map_t::mapped_type follower = pfollow->second;
		if(follower->media_mask & MEDIA_MASK_VIDEO)
		{
			pj_ssize_t len = video_package.size();
			g_safe_client_sock.Sendto(&video_package[0], &len, follower->ip, follower->port);
		}
	}
}
