#include "DisconnectScene.h"
#include "RoomMgr.h"
scene_opt_t DisconnectScene::Maintain(Termination *termination, RoomMgr* roommgr)
{
	if (termination)
	{
		RoomMgr::mtu32list	loroomlist;
		pj_uint32_t client_id = -1;
		roommgr->DeleteRoomClient_Sockfd(termination->tcp_socket_, client_id, loroomlist);
		if (termination->pfunction_)
		{
			delete termination->pfunction_;
			termination->pfunction_ = nullptr;
		}
		delete termination;
		termination = nullptr;

		if (loroomlist.size() > 0)
		{
			RoomMgr::mtu32listItr	lItr = loroomlist.begin();
			while (lItr != loroomlist.end())
			{
				roommgr->DeleteClientRoom(client_id, *lItr);
				roommgr->Logout(NULL, 0, *lItr);
				++lItr;
			}
		}
	}

	return SCENE_OPT_NONE;
}
