#include "Termination.h"

Termination::Termination(pj_sock_t tcp_socket)
	: udp_addr_(NULL)
	, tcp_ev_(NULL)
	, tcp_socket_(tcp_socket)
	, unique_id_(0)
	, media_mask_(0x00u)
	, active_(PJ_FALSE)
	, tcp_storage_offset_(0)
{
}

Termination::~Termination()
{
}

void Termination::OnLogin(uint16_t unique_id, int32_t media_port)
{
	unique_id_ = unique_id;
	media_port_ = media_port;
	active_ = PJ_TRUE;
}

void Termination::OnLogout(uint16_t unique_id)
{
	active_ = PJ_FALSE;
}