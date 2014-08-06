#include "UnlinkRoomUserScene.h"

UnlinkRoomUserParameter::UnlinkRoomUserParameter(const pj_uint8_t *storage)
: Parameter(storage)
{
	room_id_ = (pj_int32_t)pj_ntohl(*(pj_int32_t *)(storage + sizeof(Parameter)));
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)(storage + sizeof(room_id_)+sizeof(Parameter)));
	unlink_media_mask_ = *(pj_uint8_t *)(storage + sizeof(user_id_)+sizeof(room_id_)+sizeof(Parameter));
}

void UnlinkRoomUserScene::Maintain(Parameter *parameter, Termination *termination, Room *room)
{
	UnlinkRoomUserParameter *param = reinterpret_cast<UnlinkRoomUserParameter *>(parameter);
}