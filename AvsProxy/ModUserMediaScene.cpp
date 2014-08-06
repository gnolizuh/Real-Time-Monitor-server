#include "ModUserMediaScene.h"

ModUserMediaParameter::ModUserMediaParameter(const pj_uint8_t *storage)
: UdpParameter(storage)
{
	const pj_uint8_t *tmp_storage = storage + sizeof(UdpParameter);
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)tmp_storage); tmp_storage += sizeof(pj_int64_t);
	audio_ssrc_ = (pj_uint32_t)pj_ntohl(*(pj_uint32_t *)tmp_storage); tmp_storage += sizeof(pj_uint32_t);
	video_ssrc_ = (pj_uint32_t)pj_ntohl(*(pj_uint32_t *)tmp_storage);
}

void ModUserMediaScene::Maintain(UdpParameter *parameter, Room *room)
{
	RETURN_IF_FAIL( parameter != nullptr && room != nullptr );

	ModUserMediaParameter *param = reinterpret_cast<ModUserMediaParameter *>(parameter);

	room->Mod(param->user_id_, param->audio_ssrc_, param->video_ssrc_);
}
