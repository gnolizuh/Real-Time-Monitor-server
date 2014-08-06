#include "LinkRoomUserScene.h"

LinkRoomUserParameter::LinkRoomUserParameter(const pj_uint8_t *storage)
: TcpParameter(storage)
{
	room_id_ = (pj_int32_t)pj_ntohl(*(pj_int32_t *)(storage + sizeof(TcpParameter)));
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)(storage + sizeof(room_id_)+sizeof(TcpParameter)));
	link_media_mask_ = *(pj_uint8_t *)(storage + sizeof(user_id_) + sizeof(room_id_) + sizeof(TcpParameter));
}

void LinkRoomUserScene::Maintain(TcpParameter *parameter, Termination *termination, Room *room)
{
	LinkRoomUserParameter *param = reinterpret_cast<LinkRoomUserParameter *>(parameter);
	RoomUser *user = room->GetUser(param->user_id_);
}
