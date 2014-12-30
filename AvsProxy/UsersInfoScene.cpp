#include "UsersInfoScene.h"
#include "RoomMgr.h"
extern Config g_proxy_config;

UsersInfoParameter::UsersInfoParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_count_);
	users_.resize(user_count_);

	for(pj_uint8_t idx = 0; idx < user_count_; ++ idx)
	{
		pj_ntoh_assign(storage, storage_len, users_[idx].user_id);
		pj_ntoh_assign(storage, storage_len, users_[idx].mic_id);
		pj_ntoh_assign(storage, storage_len, users_[idx].audio_ssrc);
		pj_ntoh_assign(storage, storage_len, users_[idx].video_ssrc);
	}
}

scene_opt_t UsersInfoScene::Maintain(shared_ptr<UdpParameter> ptr_udp_param, RoomMgr *roommgr, pj_buffer_t &buffer)
{
	UsersInfoParameter *param = reinterpret_cast<UsersInfoParameter *>(ptr_udp_param.get());
	Room* lproom = NULL;
	lproom = roommgr->GetRoom(param->room_id_);
	if (lproom != NULL)
	{
		PJ_LOG(5, ("UsersInfoScene", "Room id %d user count %d", param->room_id_, param->user_count_));
		for (pj_uint8_t idx = 0; idx < param->user_count_; ++idx)
		{

			lproom->OnAddUser(param->users_[idx].user_id,
				param->users_[idx].mic_id,
				param->users_[idx].audio_ssrc,
				param->users_[idx].video_ssrc);
			PJ_LOG(5, ("UsersInfoScene", "\t user id %lld mic id %d audio ssrc %d video ssrc %d", param->users_[idx].user_id,
				param->users_[idx].mic_id,
				param->users_[idx].audio_ssrc,
				param->users_[idx].video_ssrc));
		}
		lproom->SetRoomState(Room::_enAvaliable);
		roommgr->SendRoomUserInfo(param->room_id_);
	}
	else
	{
		PJ_LOG(5, ("UsersInfoScene", "Alert! Room was not exist. room id %d user count %d", param->room_id_, param->user_count_));
	}
	return SCENE_OPT_NONE;
}
