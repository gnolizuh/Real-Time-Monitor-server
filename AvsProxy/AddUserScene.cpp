#include "AddUserScene.h"

AddUserParameter::AddUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_id_);
}

scene_opt_t AddUserScene::Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer)
{
	AddUserParameter *param = reinterpret_cast<AddUserParameter *>(ptr_udp_param.get());

	pj_status_t status;
	status = room->OnAddUser(param->user_id_);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, SCENE_OPT_NONE);

	request_to_client_room_add_user_t room_add_user;
	room_add_user.client_request_type = REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_ADD_USER;
	room_add_user.proxy_id = param->proxy_id_;
	room_add_user.room_id = param->room_id_;
	room_add_user.user_id = param->user_id_;

	pj_uint8_t *proom_add_user = (pj_uint8_t *)&room_add_user;
	buffer.assign(proom_add_user, proom_add_user + sizeof(room_add_user));

	return SCENE_OPT_TCP_TO_CLIENT;
}
