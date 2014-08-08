#include "Room.h"

pj_status_t Room::OnLinkRoomUser(pj_int64_t user_id, pj_uint8_t proxy_id, const pj_str_t &ip, pj_int32_t port, pj_uint8_t media_mask)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	RETURN_VAL_IF_FAIL(puser != online_users_.end(), PJ_ENOTFOUND);

	user_map_t::mapped_type user = puser->second;
	RETURN_VAL_IF_FAIL(user != nullptr, PJ_ENOTFOUND);

	user->OnLinkRoomUser(proxy_id, ip, port, media_mask);

	return PJ_SUCCESS;
}

pj_status_t Room::OnUnlinkRoomUser(pj_int64_t user_id, pj_uint8_t proxy_id)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	RETURN_VAL_IF_FAIL(puser != online_users_.end(), PJ_ENOTFOUND);

	user_map_t::mapped_type user = puser->second;
	RETURN_VAL_IF_FAIL(user != nullptr, PJ_ENOTFOUND);

	user->OnUnlinkRoomUser(proxy_id);

	return PJ_SUCCESS;
}

RoomUser *Room::GetUser(pj_int64_t user_id)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	return puser->second;
}

pj_status_t Room::OnAddOnlineUser(pj_int64_t user_id, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc)
{
	user_map_t::iterator puser = online_users_.find(user_id);
	RETURN_VAL_IF_FAIL( puser == online_users_.end(), PJ_EEXISTS );

	user_map_t::mapped_type user = new RoomUser();
	user->user_id_ = user_id;

	online_users_[user_id] = user;

	return PJ_SUCCESS;
}

pj_status_t Room::OnAddOnlineUser(pj_int64_t user_id)
{
	return OnAddOnlineUser(user_id, 0, 0);
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
		delete user;
		user = NULL;
	}
	else
	{
		return PJ_EINVAL;
	}

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