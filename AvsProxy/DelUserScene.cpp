#include "DelUserScene.h"

DelUserParameter::DelUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_id_);
}

void DelUserScene::Maintain(UdpParameter *parameter, Room *room)
{
	RETURN_IF_FAIL( parameter != nullptr && room != nullptr );

	DelUserParameter *param = reinterpret_cast<DelUserParameter *>(parameter);

	room->DelUser(param->user_id_);
}
