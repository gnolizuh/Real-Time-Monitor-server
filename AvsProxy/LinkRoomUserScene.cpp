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

	RoomUser *user = room->GetUser(param->user_id_);
	const pj_str_t term_ip = termination->GetIp();
	pj_int32_t term_media_port = param->user_media_port_;
	pj_int64_t user_id = param->user_id_;

}
