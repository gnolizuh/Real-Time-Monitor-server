#include "ModUserMediaScene.h"

ModUserMediaParameter::ModUserMediaParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, audio_ssrc_);
	pj_ntoh_assign(storage, storage_len, video_ssrc_);
}

scene_opt_t ModUserMediaScene::Maintain(UdpParameter *parameter, Room *room, pj_buffer_t &buffer)
{
	ModUserMediaParameter *param = reinterpret_cast<ModUserMediaParameter *>(parameter);

	pj_status_t status;
	status = room->OnModUser(param->user_id_, param->audio_ssrc_, param->video_ssrc_);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, SCENE_OPT_NONE);

	request_to_client_room_mod_media_t room_mod_user;
	room_mod_user.client_request_type = REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_MOD_MEDIA;
	room_mod_user.proxy_id = param->proxy_id_;
	room_mod_user.room_id = param->room_id_;
	room_mod_user.user_id = param->user_id_;
	room_mod_user.audio_ssrc = param->audio_ssrc_;
	room_mod_user.video_ssrc = param->video_ssrc_;

	pj_uint8_t *proom_mod_user = (pj_uint8_t *)&room_mod_user;
	buffer.assign(proom_mod_user, proom_mod_user + sizeof(room_mod_user));

	return SCENE_OPT_TCP_TO_CLIENT;
}
