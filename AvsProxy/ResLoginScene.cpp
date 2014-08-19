#include "ResLoginScene.h"

ResLoginParameter::ResLoginParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, response_code_);
}

scene_opt_t ResLoginScene::Maintain(UdpParameter *parameter, Room *room, pj_buffer_t &buffer)
{
	ResLoginParameter *param = reinterpret_cast<ResLoginParameter *>(parameter);
	room->OnRxLogin(param->response_code_ == 1);

	return SCENE_OPT_NONE;
}
