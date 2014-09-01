#include "ResKeepAliveScene.h"

ResKeepAliveParameter::ResKeepAliveParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
}

scene_opt_t ResKeepAliveScene::Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer)
{
	ResKeepAliveParameter *param = reinterpret_cast<ResKeepAliveParameter *>(ptr_udp_param.get());

	room->OnRxKeepAlive();

	return SCENE_OPT_NONE;
}
