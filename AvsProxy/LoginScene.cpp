#include "LoginScene.h"

extern Config g_proxy_config;

LoginParameter::LoginParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	media_ip_ = *(pj_in_addr *)storage; storage += sizeof(pj_in_addr); storage_len -= sizeof(pj_in_addr);
	pj_ntoh_assign(storage, storage_len, media_port_);
}

scene_opt_t LoginScene::Maintain(TcpParameter *parameter, Termination *termination)
{
	LoginParameter *param = reinterpret_cast<LoginParameter *>(parameter);

	termination->OnLogin(param->client_id_, param->media_ip_, param->media_port_);

	return SCENE_OPT_NONE;
}
