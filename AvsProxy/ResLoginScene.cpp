#include "ResLoginScene.h"

ResLoginParameter::ResLoginParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, response_code_);
}

void ResLoginScene::Maintain(UdpParameter *parameter, RoomMgr *mgr)
{
	RETURN_IF_FAIL( parameter != nullptr && mgr != nullptr );

	ResLoginParameter *param = reinterpret_cast<ResLoginParameter *>(parameter);
	/*room->OnLogin(param->response_code_ == 1);*/
}
