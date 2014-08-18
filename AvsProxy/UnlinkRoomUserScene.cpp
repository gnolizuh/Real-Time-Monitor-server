#include "UnlinkRoomUserScene.h"

UnlinkRoomUserParameter::UnlinkRoomUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, unlink_media_mask_);
}

void UnlinkRoomUserScene::Maintain(TcpParameter *parameter, Termination *termination, RoomMgr *mgr)
{
	UnlinkRoomUserParameter *param = reinterpret_cast<UnlinkRoomUserParameter *>(parameter);

	/*room->OnUnlinkUser(param->user_id_, param->client_id_);*/
}