#include "Termination.h"

Termination::Termination(const pj_str_t &ip, pj_sock_t tcp_socket)
	: ip_(pj_str(ip.ptr))
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

void Termination::OnLogin(pj_uint16_t unique_id)
{
	unique_id_ = unique_id;
	active_ = PJ_TRUE;
}

void Termination::OnLogout(pj_uint16_t unique_id)
{
	active_ = PJ_FALSE;
}