#include "LinkRoomUserScene.h"

LinkRoomUserParameter::LinkRoomUserParameter(const pj_uint8_t *storage)
: Parameter(storage)
{
	room_id_ = (pj_int32_t)pj_ntohl(*(pj_int32_t *)(storage + sizeof(Parameter)));
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)(storage + sizeof(room_id_)+sizeof(Parameter)));
	link_media_mask_ = *(pj_uint8_t *)(storage + sizeof(user_id_) + sizeof(room_id_) + sizeof(Parameter));
}

void LinkRoomUserScene::Maintain(Parameter *parameter, Termination *termination, Room *room)
{
	LinkRoomUserParameter *param = reinterpret_cast<LinkRoomUserParameter *>(parameter);
	RoomUser *user = room->GetUser(param->user_id_);
}
