#include "LinkRoomScene.h"
#include "RoomMgr.h"
LinkRoomParameter::LinkRoomParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
:TcpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, room_id);
}

CLinkRoomScene::CLinkRoomScene()
{
}


CLinkRoomScene::~CLinkRoomScene()
{
}

scene_opt_t CLinkRoomScene::Maintain(shared_ptr<TcpParameter> ptr_tcp_param, RoomMgr* roommgr, Termination *termination)
{
	LinkRoomParameter *param = reinterpret_cast<LinkRoomParameter *>(ptr_tcp_param.get());
	pj_uint32_t	ip = 0;
	pj_uint16_t port = 0;
	PJ_LOG(5, ("CLinkRoomScene", "Client id %d Room id %d.", param->client_id_, param->room_id));
	Room *lproom = roommgr->GetRoom(param->room_id);
	if (lproom == NULL)
	{
		if (roommgr->GetAvsStatusInfo(param->room_id, ip, port))
		{
			roommgr->UpdateRoomClientList(param->room_id, termination->client_id_, termination->tcp_socket_);
			roommgr->UpdateClientRoomList(termination->client_id_, param->room_id);
			pj_in_addr addr;
			addr.s_addr = pj_ntohl(ip);
			char     dst_ip[32];
			pj_str_t dst_str_ip = {dst_ip, sizeof(dst_ip)};
			pj_str_t src_str_ip = pj_str(pj_inet_ntoa(addr));
			pj_strncpy(&dst_str_ip, &src_str_ip, dst_str_ip.slen - 1);

			roommgr->Login(&dst_str_ip, port, param->room_id);
		}
		else
		{
			PJ_LOG(5, ("CLinkRoomScene", "Fatal! Could not get the room %d AVS status info.", param->room_id));
		}
	
	}
	else
	{
		if (lproom->GetCurRoomState() == Room::_enAvaliable)
		{
			roommgr->UpdateRoomClientList(param->room_id, termination->client_id_, termination->tcp_socket_);
			roommgr->UpdateClientRoomList(termination->client_id_, param->room_id);

			roommgr->SendRoomUserInfo(param->room_id);
		}
		else if (lproom->GetCurRoomState() == Room::_enBuilding)
		{
			roommgr->UpdateRoomClientList(param->room_id, termination->client_id_, termination->tcp_socket_);
			roommgr->UpdateClientRoomList(termination->client_id_, param->room_id);

			pj_in_addr addr;
			addr.s_addr = pj_ntohl(ip);
			char     dst_ip[32];
			pj_str_t dst_str_ip = {dst_ip, sizeof(dst_ip)};
			pj_str_t src_str_ip = pj_str(pj_inet_ntoa(addr));
			pj_strncpy(&dst_str_ip, &src_str_ip, dst_str_ip.slen - 1);
			roommgr->Login(&dst_str_ip, port, param->room_id);			
		}
		else
		{
			PJ_LOG(5, ("CLinkRoomScene", "Exception! The room %d state is error.", param->room_id));
		}
		
	}

	
	return SCENE_OPT_NONE;
}