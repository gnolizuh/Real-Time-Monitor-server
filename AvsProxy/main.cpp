#include <iostream>

#include "RoomMgr.h"

int main(int argc, char **argv)
{
	pj_status_t status;
	pj_str_t avsproxy_ip = pj_str("192.168.1.108");
	pj_uint16_t avsproxy_tcp_port = 12000;
	pj_uint16_t avsproxy_udp_port = 13000;
	RoomMgr mgr(avsproxy_ip, avsproxy_tcp_port, avsproxy_udp_port, 10);

	status = pj_init();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	pj_str_t log_file_name = pj_str("avsproxy.log");
	status = mgr.Prepare(log_file_name);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = mgr.Launch();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	PJ_LOG(5, (__FILE__, "AvsProxy start........"));

	pj_str_t avs_ip = pj_str("183.136.132.47");
	pj_uint16_t avs_port = 8065;
	pj_int32_t room_id = 462728;

	// mgr.Login(&avs_ip, avs_port, room_id);

	int i;
	std::cin >> i;

	// mgr.Logout(&avs_ip, avs_port, room_id);

	mgr.Destroy();

	return 1;
}
