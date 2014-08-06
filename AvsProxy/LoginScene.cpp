#include "LoginScene.h"

LoginParameter::LoginParameter(const pj_uint8_t *storage)
	: Parameter(storage)
{
	media_port_ = ntohl(*(pj_int32_t *)(storage + sizeof(Parameter)));
}

void LoginScene::Maintain(Parameter *parameter, Termination *termination, Room *room)
{
	LoginParameter *param = reinterpret_cast<LoginParameter *>(parameter);

	termination->OnLogin(param->client_id_, param->media_port_);
}
