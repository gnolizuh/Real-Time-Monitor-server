#include "Termination.h"

Termination::Termination(pj_sock_t tcp_socket)
	: tcp_ev_(NULL)
	, tcp_socket_(tcp_socket)
	, client_id_(0)
	, media_mask_(0x00u)
	, active_(PJ_FALSE)
	, tcp_storage_offset_(0)
	, linked_rooms_()
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

void Termination::OnLink(room_id_t room_id, pj_uint64_t user_id)
{
	linked_rooms_t::iterator proom = linked_rooms_.find(room_id);
	linked_rooms_t::mapped_type users;
	if(proom == linked_rooms_.end())
	{
		users.insert(user_id);

		linked_rooms_.insert(linked_rooms_t::value_type(room_id, users));
	}
	else
	{
		if(proom->second.find(user_id) == proom->second.end())
		{
			proom->second.insert(user_id);
		}
	}
}

void Termination::OnUnlink(room_id_t room_id, pj_uint64_t user_id)
{
	linked_rooms_t::iterator proom = linked_rooms_.find(room_id);
	if(proom != linked_rooms_.end())
	{
		room_users_t::iterator puser = proom->second.find(user_id);
		if(puser != proom->second.end())
		{
			proom->second.erase(puser);
		}

		if(proom->second.empty())
		{
			linked_rooms_.erase(proom);
		}
	}
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