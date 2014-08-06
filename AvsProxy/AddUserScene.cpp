#include "AddUserScene.h"

AddUserParameter::AddUserParameter(const pj_uint8_t *storage)
: UdpParameter(storage)
{
	const pj_uint8_t *tmp_storage = storage + sizeof(UdpParameter);
	user_id_ = (pj_int64_t)pj_ntohll(*(pj_int64_t *)tmp_storage);
}

void AddUserScene::Maintain(UdpParameter *parameter, Room *room)
{
	RETURN_IF_FAIL( parameter != nullptr && room != nullptr );

	AddUserParameter *param = reinterpret_cast<AddUserParameter *>(parameter);

	room->Add(param->user_id_, 0, 0);
}
