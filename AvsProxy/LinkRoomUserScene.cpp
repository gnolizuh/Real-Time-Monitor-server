#include "LinkRoomUserScene.h"

LinkRoomUserParameter::LinkRoomUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, user_media_port_);
	pj_ntoh_assign(storage, storage_len, link_media_mask_);
}

void LinkRoomUserScene::Maintain(TcpParameter *parameter, Termination *termination, Room *room)
{
	LinkRoomUserParameter *param = reinterpret_cast<LinkRoomUserParameter *>(parameter);

	room->OnLinkRoomUser(param->user_id_, param->proxy_id_ , termination->GetIp(), param->user_media_port_, param->link_media_mask_);
}
