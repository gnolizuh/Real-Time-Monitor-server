#include "Room.h"

Room::Room(const pj_str_t &room_ip,
		   pj_uint16_t room_port,
		   pj_sock_t &ref_udp_sock,
		   pjmedia_rtp_session &ref_rtp_session,
		   mutex &ref_rtp_sess_lock)
	: room_ip_(pj_str(room_ip.ptr))
	, room_port_(room_port)
	, ref_udp_sock_(ref_udp_sock)
	, ref_rtp_session_(ref_rtp_session)
	, ref_rtp_sess_lock_(ref_rtp_sess_lock)
{
}

Room::~Room()
{
}

pj_status_t Room::Prepare()
{
	return PJ_SUCCESS;
}

pj_status_t Room::Launch()
{
	return PJ_SUCCESS;
}

void Room::Destory()
{
	lock_guard<mutex> lock(room_lock_);
	user_map_t::iterator puser = online_users_.begin();
	for(; puser != online_users_.end(); ++ puser)
	{
		user_map_t::mapped_type user = puser->second;
		if(user != nullptr)
		{
			user->Destory();
			delete user;
			user = nullptr;
		}
		puser = online_users_.erase(puser);
	}
}

user_map_t &Room::GetUsers()
{
	return online_users_;
}

pj_status_t Room::OnLinkUser(pj_int64_t user_id,
							 pj_uint16_t proxy_id,
							 const pj_str_t &ip,
							 pj_uint16_t port,
							 pj_uint8_t media_mask,
							 pj_bool_t &is_continue)
{
	lock_guard<mutex> lock(room_lock_);
	user_map_t::iterator puser = online_users_.find(user_id);
	RETURN_VAL_IF_FAIL(puser != online_users_.end(), PJ_ENOTFOUND);

	user_map_t::mapped_type user = puser->second;
	RETURN_VAL_IF_FAIL(user != nullptr, PJ_ENOTFOUND);

	user->OnLink(proxy_id, ip, port, media_mask, is_continue);

	return PJ_SUCCESS;
}

pj_status_t Room::OnUnlinkUser(pj_int64_t user_id,
							   pj_uint16_t proxy_id,
							   pj_bool_t &is_continue)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	RETURN_VAL_IF_FAIL(puser != online_users_.end(), PJ_ENOTFOUND);

	user_map_t::mapped_type user = puser->second;
	RETURN_VAL_IF_FAIL(user != nullptr, PJ_ENOTFOUND);

	user->OnUnlink(proxy_id, is_continue);

	return PJ_SUCCESS;
}

RoomUser *Room::GetUser(pj_int64_t user_id)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	return puser->second;
}

pj_status_t Room::OnAddUser(pj_int64_t user_id, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	RETURN_VAL_IF_FAIL( puser == online_users_.end(), PJ_EEXISTS );

	user_map_t::mapped_type user = new RoomUser();
	user->user_id_ = user_id;
	user->audio_ssrc_ = audio_ssrc;
	user->video_ssrc_ = video_ssrc;

	online_users_[user_id] = user;

	return PJ_SUCCESS;
}

pj_status_t Room::OnAddUser(pj_int64_t user_id)
{
	return OnAddUser(user_id, 0, 0);
}

pj_status_t Room::OnModUser(pj_int64_t user_id, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	user_map_t::mapped_type user = puser->second;
	if (puser != online_users_.end() && user != nullptr)
	{
		user->audio_ssrc_ = audio_ssrc;
		user->video_ssrc_ = video_ssrc;
	}
	else
	{
		return PJ_EINVAL;
	}

	return PJ_SUCCESS;
}

pj_status_t Room::OnDelUser(pj_int64_t user_id)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	user_map_t::mapped_type user = puser->second;
	if (puser != online_users_.end() && user != nullptr)
	{
		online_users_.erase(puser);
		user->Destory();
		delete user;
		user = NULL;
	}
	else
	{
		return PJ_EINVAL;
	}

	return PJ_SUCCESS;
}

pj_status_t Room::OnRxLogin(pj_bool_t opt)
{
	return PJ_SUCCESS;
}

pj_status_t Room::OnRxKeepAlive()
{
	return PJ_SUCCESS;
}

void Room::OnRxRtp(pj_int64_t user_id, pj_uint8_t media_type, const vector<pj_uint8_t> &rtp_package)
{
	RoomUser *room_user = GetUser(user_id);
	if ( room_user != nullptr )
	{
		media_type == MEDIA_MASK_AUDIO ? room_user->OnRxAudio(rtp_package) : room_user->OnRxVideo(rtp_package);
	}
}

pj_status_t Room::SendRTPPacket(const void *payload, pj_ssize_t *payload_len)
{
	RETURN_VAL_IF_FAIL(*payload_len > 0, PJ_EINVAL);

	pj_uint8_t packet[MAX_UDP_DATA_SIZE];
	const pjmedia_rtp_hdr *hdr;
	const void *p_hdr;
	int hdrlen;
	
	lock_guard<mutex> lock(ref_rtp_sess_lock_);
	pj_status_t status;
	status = pjmedia_rtp_encode_rtp (&ref_rtp_session_, RTP_EXPAND_PAYLOAD_TYPE, 0,
		*payload_len, 0, &p_hdr, &hdrlen);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	hdr = (const pjmedia_rtp_hdr*) p_hdr;

	/* Copy RTP header to packet */
	pj_memcpy(packet, hdr, hdrlen);

	/* Copy RTP payload to packet */
	pj_memcpy(packet + hdrlen, payload, *payload_len);

	/* Send RTP packet */
	*payload_len += hdrlen;

	pj_sockaddr_in addr;
	status = pj_sockaddr_in_init(&addr, &room_ip_, room_port_);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	return pj_sock_sendto(ref_udp_sock_, packet, payload_len, 0, &addr, sizeof(addr));
}

