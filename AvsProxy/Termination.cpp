#include "Termination.h"

Termination::Termination(const pj_str_t &ip, pj_sock_t tcp_socket)
	: ip_(pj_str(ip.ptr))
	, tcp_ev_(NULL)
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

void Termination::OnLogin(pj_uint16_t client_id,
						  pj_uint16_t media_port)
{
	client_id_ = client_id;
	media_port_ = media_port;
	active_ = PJ_TRUE;
}

void Termination::OnLogout(pj_uint16_t unique_id)
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