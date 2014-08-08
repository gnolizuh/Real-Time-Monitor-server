#include "ModUserMediaScene.h"

ModUserMediaParameter::ModUserMediaParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, audio_ssrc_);
	pj_ntoh_assign(storage, storage_len, video_ssrc_);
}

void ModUserMediaScene::Maintain(UdpParameter *parameter, Room *room)
{
	RETURN_IF_FAIL( parameter != nullptr && room != nullptr );

	ModUserMediaParameter *param = reinterpret_cast<ModUserMediaParameter *>(parameter);

	room->ModUser(param->user_id_, param->audio_ssrc_, param->video_ssrc_);
}
