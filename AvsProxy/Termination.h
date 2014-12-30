#ifndef __AVS_PROXY_TERMINATION__
#define __AVS_PROXY_TERMINATION__

#include <set>
#include <map>
#include "ClientStructs.h"
#include "Com.h"

using std::set;
using std::map;

#define INVALID_CLIENT_ID  0
#define INVALID_MEDIA_PORT 0

typedef set<pj_uint64_t> room_users_t;
typedef map<room_id_t, room_users_t> linked_rooms_t;
class Termination
{
public:
	Termination(pj_sock_t);
	~Termination();

	inline pj_str_t GetIp() const { return ip_; }
	inline pj_uint16_t GetMediaPort() const { return media_port_; }

	pj_uint16_t GetClientID();
	pj_uint16_t GetMediaPort();
	void OnLogin(pj_uint16_t client_id, pj_in_addr media_ip, pj_uint16_t media_port);
	void OnLogout(pj_uint16_t client_id);
	void OnLink(room_id_t room_id, pj_uint64_t user_id);
	void OnUnlink(room_id_t room_id, pj_uint64_t user_id);
	void OnConsole(stringstream &output);
	void OnKeepAlive(pj_uint16_t, pj_uint16_t);
	pj_status_t SendTCPPacket(const void *, pj_ssize_t *);

	pj_str_t       ip_;
	pj_uint16_t    media_port_;
	ev_function_t *pfunction_;
	struct event  *tcp_ev_;
	pj_sock_t      tcp_socket_;                     // 客户端套接字
	pj_uint16_t    client_id_;                      // 客户端ID
	pj_uint8_t     media_mask_;                     // 此客户端关注的媒体类型
	pj_uint8_t     status_;                         // 客户端状态
	pj_bool_t      active_;                         // 在线状态
	pj_uint8_t     tcp_storage_[MAX_STORAGE_SIZE];  // TCP缓存
	pj_uint16_t    tcp_storage_offset_;             // TCP缓存偏移地址
	linked_rooms_t linked_rooms_;                   // 当前终端所关注的房间
};

#endif
