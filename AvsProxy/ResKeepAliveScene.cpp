#include "ResKeepAliveScene.h"

ResKeepAliveParameter::ResKeepAliveParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
}

void ResKeepAliveScene::Maintain(UdpParameter *parameter, RoomMgr *mgr)
{
	RETURN_IF_FAIL( parameter != nullptr && mgr != nullptr );

	ResKeepAliveParameter *param = reinterpret_cast<ResKeepAliveParameter *>(parameter);
}
