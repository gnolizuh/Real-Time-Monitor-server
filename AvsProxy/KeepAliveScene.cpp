#include "KeepAliveScene.h"

KeepAliveParameter::KeepAliveParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
: TcpParameter(storage, storage_len)
{
}

void KeepAliveScene::Maintain(TcpParameter *parameter, Termination *termination, RoomMgr *mgr)
{
	KeepAliveParameter *param = reinterpret_cast<KeepAliveParameter *>(parameter);

	/*termination->OnKeepAlive(param->proxy_id_, param->client_id_);*/
}
