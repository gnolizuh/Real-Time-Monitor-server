#include "KeepAliveScene.h"

KeepAliveParameter::KeepAliveParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
: TcpParameter(storage, storage_len)
{
}

scene_opt_t KeepAliveScene::Maintain(TcpParameter *parameter, Termination *termination, pj_buffer_t &buffer)
{
	KeepAliveParameter *param = reinterpret_cast<KeepAliveParameter *>(parameter);

	response_to_client_keep_alive_t keep_alive;
	keep_alive.client_request_type = RESPONSE_FROM_AVSPROXY_TO_CLIENT_KEEP_ALIVE;
	keep_alive.proxy_id = param->proxy_id_;
	keep_alive.client_id = param->client_id_;

	pj_uint8_t *pkeep_alive = (pj_uint8_t *)&keep_alive;
	buffer.assign(pkeep_alive, pkeep_alive + sizeof(keep_alive));

	return SCENE_OPT_NONE;
}
