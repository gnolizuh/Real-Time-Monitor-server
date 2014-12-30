#include"UnLinkRoomScene.h"
#include"RoomMgr.h"
UnLinkRoomParameter::UnLinkRoomParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
:TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id);
	pj_ntoh_assign(storage, storage_len, user_id);
}

scene_opt_t CUnLinkRoomScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, RoomMgr* roommgr, Termination *termination)
{
	UnLinkRoomParameter *param = reinterpret_cast<UnLinkRoomParameter *>(ptr_tcp_param.get());
	// 
	bool	booret = roommgr->DeleteClientRoom(param->client_id_, param->room_id);
	if (roommgr->DeleteRoomClient(param->room_id, param->client_id_))
	{
		pj_status_t ret = roommgr->Logout(NULL,0,param->room_id);
		if (ret != PJ_SUCCESS)
		{
			PJ_LOG(5, ("CUnLinkRoomScene", "There is no client linking to room(id %d) and Proxy logouts successfully. The last client id is %d.", param->room_id, param->client_id_));
		}
		else
		{
			PJ_LOG(5, ("CUnLinkRoomScene", "Proxy logouts AVS failed. room id %d client id %d",param->room_id, param->client_id_));
		}
	}
	else
	{
		PJ_LOG(5, ("CUnLinkRoomScene", "room id %d client id %d", param->room_id, param->client_id_));
	}

	return SCENE_OPT_NONE;
}