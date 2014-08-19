#include <iostream>
#include <vector>

#include "pugixml.hpp"
#include "RoomMgr.h"

using std::vector;

typedef struct
{
	pj_str_t ip;
	pj_uint16_t port;
	pj_uint32_t avs_id;
} avs_config_t;

typedef struct config
{
	pj_str_t config_file_name;
	pj_str_t local_ip;
	pj_uint16_t local_tcp_port;
	pj_uint16_t local_udp_port;
	pj_uint8_t  thread_cnt;
	vector<avs_config_t *> avs;
} config_t;

static config_t g_config;

static pj_status_t init_options(int argc, char *argv[])
{
	return PJ_SUCCESS;
}

static pj_status_t init_param()
{
	pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_file("config.xml");
	RETURN_VAL_IF_FAIL(result, PJ_EINVAL);

	pugi::xml_node proxy = doc.child("config").child("proxy");
	g_config.local_ip = pj_str(strdup((char *)proxy.attribute("ip").value()));
	g_config.local_tcp_port = atoi(proxy.attribute("tcp_port").value());
	g_config.local_udp_port = atoi(proxy.attribute("udp_port").value());
	g_config.thread_cnt = atoi(proxy.attribute("thread_cnt").value());

	pugi::xml_node avs_configs = doc.child("config").child("avs_config");
	for(pugi::xml_node avs = avs_configs.child("avs"); avs; avs = avs.next_sibling("avs"))
	{
		avs_config_t *pavs = new avs_config_t();
		pavs->avs_id = atoi(avs.attribute("room_id").value());
		pavs->ip = pj_str(strdup((char *)avs.attribute("ip").value()));
		pavs->port = atoi(avs.attribute("port").value());
		g_config.avs.push_back(pavs);
	}

	return PJ_SUCCESS;
}

int main(int argc, char *argv[])
{
	pj_status_t status;

	status = init_options(argc, argv);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = init_param();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = pj_init();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	RoomMgr mgr(g_config.local_ip, g_config.local_tcp_port, g_config.local_udp_port, g_config.thread_cnt);

	pj_str_t log_file_name = pj_str("avsproxy.log");
	status = mgr.Prepare(log_file_name);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = mgr.Launch();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	PJ_LOG(5, (__FILE__, "AvsProxy start........"));

	pj_str_t avs_ip = pj_str("60.165.99.210");
	pj_uint16_t avs_port = 9501;
	pj_int32_t room_id = 501649;

	for(pj_uint32_t i = 0; i < g_config.avs.size(); ++ i)
	{
		mgr.Login(&g_config.avs[i]->ip, g_config.avs[i]->port, g_config.avs[i]->avs_id);
	}

	int i;
	std::cin >> i;

	for(pj_uint32_t i = 0; i < g_config.avs.size(); ++ i)
	{
		mgr.Logout(&g_config.avs[i]->ip, g_config.avs[i]->port, g_config.avs[i]->avs_id);
	}

	mgr.Destroy();

	return 1;
}
