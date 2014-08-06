#include "UnlinkRoomUserScene.h"

UnlinkRoomUserParameter::UnlinkRoomUserParameter(const pj_uint8_t *storage)
: TcpParameter(storage)
{
	room_id_ = (pj_int32_t)pj_ntohl(*(pj_int32_t *)(storage + sizeof(TcpParameter)));
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)(storage + sizeof(room_id_)+sizeof(TcpParameter)));
	unlink_media_mask_ = *(pj_uint8_t *)(storage + sizeof(user_id_)+sizeof(room_id_)+sizeof(TcpParameter));
}

void UnlinkRoomUserScene::Maintain(TcpParameter *parameter, Termination *termination, Room *room)
{
	UnlinkRoomUserParameter *param = reinterpret_cast<UnlinkRoomUserParameter *>(parameter);
}