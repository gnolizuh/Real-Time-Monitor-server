#include <iostream>

#include "RoomMgr.h"

int main(int argc, char **argv)
{
	pj_status_t status;
	char ip[64] = "192.168.6.38";
	PoolThread<std::function<void ()>> pool_thread(5);
	RoomMgr mgr(ip, 12000, 13000, 10);

	status = pj_init();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = mgr.Prepare();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = mgr.Launch();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	char c_avs_ip[64] = "183.136.132.47";
	pj_str_t avs_ip = pj_str(c_avs_ip);
	pj_uint16_t avs_port = 8065;
	mgr.Login(&avs_ip, avs_port);

	int i;
	std::cin >> i;

	mgr.Logout(&avs_ip, avs_port);

	return 1;
}
