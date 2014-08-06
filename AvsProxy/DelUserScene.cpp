#include "DelUserScene.h"

DelUserParameter::DelUserParameter(const pj_uint8_t *storage)
: UdpParameter(storage)
{
	const pj_uint8_t *tmp_storage = storage + sizeof(UdpParameter);
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)tmp_storage);
}

void DelUserScene::Maintain(UdpParameter *parameter, Room *room)
{
	RETURN_IF_FAIL( parameter != nullptr && room != nullptr );

	DelUserParameter *param = reinterpret_cast<DelUserParameter *>(parameter);

	room->Del(param->user_id_);
}
