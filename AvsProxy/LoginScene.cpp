#include "LoginScene.h"

LoginParameter::LoginParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, media_port_);
}

void LoginScene::Maintain(TcpParameter *parameter, Termination *termination, RoomMgr *mgr)
{
	LoginParameter *param = reinterpret_cast<LoginParameter *>(parameter);

	termination->OnLogin(param->client_id_, param->media_port_);

	// mgr->SendUserInfoToTerm(termination);
}
