#include "LogoutScene.h"

LogoutParameter::LogoutParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
}

scene_opt_t LogoutScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Termination *termination, pj_buffer_t &buffer)
{
	LogoutParameter *param = reinterpret_cast<LogoutParameter *>(ptr_tcp_param.get());

	termination->OnLogout(param->client_id_);

	return SCENE_OPT_NONE;
}
