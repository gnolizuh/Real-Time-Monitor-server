#include "LoginScene.h"

LoginParameter::LoginParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
}

void LoginScene::Maintain(TcpParameter *parameter, Termination *termination, Room *room)
{
	LoginParameter *param = reinterpret_cast<LoginParameter *>(parameter);

	termination->OnLogin(param->client_id_);
}
