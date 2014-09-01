#include "LinkRoomUserScene.h"

LinkRoomUserParameter::LinkRoomUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, link_media_mask_);
}

scene_opt_t LinkRoomUserScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, Room *room, Termination *termination, pj_buffer_t &buffer)
{
	LinkRoomUserParameter *param = reinterpret_cast<LinkRoomUserParameter *>(ptr_tcp_param.get());

	pj_bool_t is_continue = PJ_FALSE;
	room->OnLinkUser(param->user_id_,
		param->proxy_id_,
		termination->GetIp(),
		termination->GetMediaPort(),
		param->link_media_mask_,
		is_continue);
	RETURN_VAL_IF_FAIL(is_continue == PJ_TRUE, SCENE_OPT_NONE);

	request_to_avs_link_user_t link_user;
	link_user.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LINK_USER;
	link_user.proxy_id = param->proxy_id_;
	link_user.room_id = param->room_id_;
	link_user.user_id = param->user_id_;
	link_user.link_media_mask = param->link_media_mask_;

	pj_uint8_t *plink_user = (pj_uint8_t *)&link_user;
	buffer.assign(plink_user, plink_user + sizeof(link_user));

	return SCENE_OPT_RTP_TO_AVS;
}
