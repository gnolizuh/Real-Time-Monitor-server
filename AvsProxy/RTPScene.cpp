#include "RTPScene.h"

RTPParameter::RTPParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, ssrc_);
	pj_ntoh_assign(storage, storage_len, media_type_);
	rtp_package_.assign(storage, storage + storage_len);
}

scene_opt_t RTPScene::Maintain(UdpParameter *parameter, Room *room, pj_buffer_t &buffer)
{
	RTPParameter *param = reinterpret_cast<RTPParameter *>(parameter);
	room->OnRxRtp(param->user_id_, param->media_type_, param->rtp_package_);

	return SCENE_OPT_NONE;
}