#include "DelUserScene.h"

DelUserParameter::DelUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, user_id_);
}

scene_opt_t DelUserScene::Maintain(shared_ptr<UdpParameter> ptr_udp_param, Room *room, pj_buffer_t &buffer)
{
	DelUserParameter *param = reinterpret_cast<DelUserParameter *>(ptr_udp_param.get());

	pj_status_t status;
	status = room->OnDelUser(param->user_id_);
	PJ_LOG(5, ("DelUserScene", "User id %lld ...", param->user_id_));
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, SCENE_OPT_NONE);
	PJ_LOG(5, ("DelUserScene", "User id %lld successfully.", param->user_id_));
	request_to_client_room_del_user_t room_del_user;
	room_del_user.client_request_type = REQUEST_FROM_AVSPROXY_TO_CLIENT_ROOM_DEL_USER;
	room_del_user.proxy_id = param->proxy_id_;
	room_del_user.room_id = param->room_id_;
	room_del_user.user_id = param->user_id_;
	room_del_user.Serialize();
	pj_uint8_t *proom_del_user = (pj_uint8_t *)&room_del_user;
	buffer.assign(proom_del_user, proom_del_user + sizeof(room_del_user));

	return SCENE_OPT_TCP_TO_CLIENT;
}
