#include "NATforClient.h"
#include "RoomMgr.h"

NATforClientParam::NATforClientParam(const pj_uint8_t *storage, pj_uint16_t storage_len)
:UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, client_id);
}

CNATforClient::CNATforClient()
{
}


CNATforClient::~CNATforClient()
{
}

scene_opt_t CNATforClient::Maintain(shared_ptr<UdpParameter> ptr_udp_param, RoomMgr *roommgr, pj_sockaddr_in& sockaddr_in)
{
	NATforClientParam *param = reinterpret_cast<NATforClientParam *>(ptr_udp_param.get());
	roommgr->UpdateTerminationUdpPort(param->client_id, pj_ntohs(sockaddr_in.sin_port), pj_inet_ntoa(sockaddr_in.sin_addr));
	response_to_client_nat lopack;
	lopack.client_id = param->client_id;
	lopack.client_request_type = RESPONSE_FROM_AVSPROXY_TO_CLIENT_NAT;
	lopack.proxy_id = param->proxy_id_;
	lopack.room_id = param->room_id_;
	lopack.Serialize();
	int len = sizeof(lopack);
	pj_status_t ret = roommgr->SendRTPPacket(pj_str(strdup(pj_inet_ntoa(sockaddr_in.sin_addr))), pj_ntohs(sockaddr_in.sin_port), RTP_EXPAND_PAYLOAD_TYPE, &lopack, len);
	if (ret == 0)
	{
		PJ_LOG(5, ("CNATforClient", "Success! Client id %d IP %s port %d.", param->client_id, pj_inet_ntoa(sockaddr_in.sin_addr), pj_ntohs(sockaddr_in.sin_port)));
	}
	else
	{
		PJ_LOG(5, ("CNATforClient", "Failed! Client id %d IP %s port %d.", param->client_id, pj_inet_ntoa(sockaddr_in.sin_addr), pj_ntohs(sockaddr_in.sin_port)));
	}
	return SCENE_OPT_NONE;
}
