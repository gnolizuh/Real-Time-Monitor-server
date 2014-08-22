#include "Termination.h"

Termination::Termination(pj_sock_t tcp_socket)
	: tcp_ev_(NULL)
	, tcp_socket_(tcp_socket)
	, client_id_(0)
	, media_mask_(0x00u)
	, active_(PJ_FALSE)
	, tcp_storage_offset_(0)
{
}

Termination::~Termination()
{
}

pj_uint16_t Termination::GetClientID()
{
	if(active_)
	{
		return client_id_;
	}

	return INVALID_CLIENT_ID;
}

pj_uint16_t Termination::GetMediaPort()
{
	if(active_)
	{
		return media_port_;
	}

	return INVALID_MEDIA_PORT;
}

void Termination::OnLogin(pj_uint16_t client_id,
						  pj_in_addr  media_ip,
						  pj_uint16_t media_port)
{
	ip_ = pj_str(pj_inet_ntoa(media_ip));
	client_id_ = client_id;
	media_port_ = media_port;
	active_ = PJ_TRUE;
}

void Termination::OnLogout(pj_uint16_t client_id)
{
	active_ = PJ_FALSE;
}

void Termination::OnKeepAlive(pj_uint16_t proxy_id, pj_uint16_t client_id)
{
	response_to_client_keep_alive_t res_keep_alive;
	res_keep_alive.client_request_type = RESPONSE_FROM_AVSPROXY_TO_CLIENT_KEEP_ALIVE;
	res_keep_alive.proxy_id = proxy_id;
	res_keep_alive.client_id = client_id;
	res_keep_alive.Serialize();

	pj_ssize_t sndlen = sizeof(res_keep_alive);
	SendTCPPacket(&res_keep_alive, &sndlen);
}

pj_status_t Termination::SendTCPPacket(const void *buf, pj_ssize_t *len)
{
	return pj_sock_send(tcp_socket_, buf, len, 0);
}