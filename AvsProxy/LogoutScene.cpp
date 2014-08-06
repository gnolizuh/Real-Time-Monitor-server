#include "LogoutScene.h"

LogoutParameter::LogoutParameter(const pj_uint8_t *storage)
	: TcpParameter(storage)
{
}

void LogoutScene::Maintain(TcpParameter *parameter, Termination *termination, Room *room)
{
	LogoutParameter *param = reinterpret_cast<LogoutParameter *>(parameter);

	termination->OnLogout(param->client_id_);
}
