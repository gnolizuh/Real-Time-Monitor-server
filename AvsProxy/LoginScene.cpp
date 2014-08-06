#include "LoginScene.h"

LoginParameter::LoginParameter(const pj_uint8_t *storage)
	: TcpParameter(storage)
{
	media_port_ = ntohl(*(pj_int32_t *)(storage + sizeof(TcpParameter)));
}

void LoginScene::Maintain(TcpParameter *parameter, Termination *termination, Room *room)
{
	LoginParameter *param = reinterpret_cast<LoginParameter *>(parameter);

	termination->OnLogin(param->client_id_, param->media_port_);
}
