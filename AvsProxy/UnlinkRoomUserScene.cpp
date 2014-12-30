#include "UnlinkRoomUserScene.h"
#include "RoomMgr.h"
UnlinkRoomUserParameter::UnlinkRoomUserParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
	: TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id_);
	pj_ntoh_assign(storage, storage_len, user_id_);
	pj_ntoh_assign(storage, storage_len, unlink_media_mask_);
}

scene_opt_t UnlinkRoomUserScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, RoomMgr *roommgr, Termination *termination, pj_buffer_t &buffer)
{
	UnlinkRoomUserParameter *param = reinterpret_cast<UnlinkRoomUserParameter *>(ptr_tcp_param.get());
	pj_bool_t is_continue = PJ_FALSE;
	Room* lproom = NULL;
	lproom = roommgr->GetRoom(param->room_id_);
	if (lproom != NULL)
	{
		lproom->OnUnlinkUser(param->user_id_, param->proxy_id_, is_continue);
		PJ_LOG(5, ("UnlinkRoomUserScene", "room id %d client id %d user id %lld.", param->room_id_, param->client_id_, param->user_id_));
		RETURN_VAL_IF_FAIL(is_continue == PJ_TRUE, SCENE_OPT_NONE);
		PJ_LOG(5, ("UnlinkRoomUserScene", "Success! room id %d client id %d user id %lld.", param->room_id_, param->client_id_, param->user_id_));
		request_to_avs_unlink_user_t unlink_user;
		unlink_user.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_UNLINK_USER;
		unlink_user.proxy_id = param->proxy_id_;
		unlink_user.room_id = param->room_id_;
		unlink_user.user_id = param->user_id_;
		unlink_user.unlink_media_mask = param->unlink_media_mask_;

		pj_uint8_t *punlink_user = (pj_uint8_t *)&unlink_user;
		buffer.assign(punlink_user, punlink_user + sizeof(unlink_user));
	}
	else
	{
		PJ_LOG(5, ("UnlinkRoomUserScene", "Fatal! room id %d client id %d user id %lld.", param->room_id_, param->client_id_, param->user_id_));
	}
	
	return SCENE_OPT_RTP_TO_AVS;
}