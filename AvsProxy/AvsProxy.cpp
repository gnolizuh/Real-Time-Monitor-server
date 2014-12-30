#include <iostream>

#include "pugixml.hpp"
#include "RoomMgr.h"
#include "Config.h"

static const char *MENU =
"\n"
"Enter menu character:\n"
"  s    Show status\n"
"  q    Quit\n"
"\n";

extern Config g_proxy_config;

static void console_main(RoomMgr &mgr)
{
    char input1[10];

    printf("%s", MENU);

    for (;;) {
		printf(">>> "); fflush(stdout);
		if (fgets(input1, sizeof(input1), stdin) == NULL) {
			puts("EOF while reading stdin, will quit now..");
			break;
		}

		switch (input1[0])
		{
			case 's':
			{
				stringstream ss;
				mgr.OnConsole(ss);
				puts(ss.str().c_str());
				break;
			}
			case 'q':
			{
				mgr.Destroy();
				return;
			}
			default:
			{
				puts("Invalid command");
				printf("%s", MENU);
				break;
			}
		}

		fflush(stdout);
    }
}


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
	g_proxy_config.proxy_id = atoi(proxy.attribute("id").value());
	g_proxy_config.local_ip = pj_str(strdup((char *)proxy.attribute("ip").value()));
	g_proxy_config.local_tcp_port = atoi(proxy.attribute("tcp_port").value());
	g_proxy_config.local_udp_port = atoi(proxy.attribute("udp_port").value());
	g_proxy_config.thread_cnt = atoi(proxy.attribute("thread_cnt").value());
	g_proxy_config.keepalive_time = atoi(proxy.attribute("keepalive_time").value());
	if (g_proxy_config.keepalive_time != 0)
	{
		g_proxy_config.keepalive_time *= 1000;
	}
	else
	{
		g_proxy_config.keepalive_time = 300000;
	}

	pugi::xml_node mongodb_client = doc.child("config").child("mongodb_avsproxy_status_info");
	g_proxy_config.mongodb_avsproxy_status_info_ip = pj_str(strdup((char *)mongodb_client.attribute("ip").value()));
	g_proxy_config.mongodb_avsproxy_status_info_port = atoi(mongodb_client.attribute("port").value());
	g_proxy_config.mongodb_avsproxy_status_info_ns = pj_str(strdup((char *)mongodb_client.attribute("ns").value()));
	g_proxy_config.mongodb_avsproxy_status_info_reconn_time_cycle = atoi(mongodb_client.attribute("reconnect_time_cycle").value())*1000;
	g_proxy_config.mongodb_avsproxy_status_info_report_time_cycle = atoi(mongodb_client.attribute("report_time_cycle").value())*1000;
	g_proxy_config.mongodb_avsproxy_status_info_rcheck_time_cycle = atoi(mongodb_client.attribute("info_recheck_time_cycle").value()) * 1000;
	g_proxy_config.mongodb_avsproxy_status_info_rcheck_time_gap = atoi(mongodb_client.attribute("info_recheck_time_gap").value()) * 1000;
	pugi::xml_node avs_configs = doc.child("config").child("avs_config");
	for(pugi::xml_node avs = avs_configs.child("avs"); avs; avs = avs.next_sibling("avs"))
	{
		avs_config_t *pavs = new avs_config_t();
		pavs->avs_id = atoi(avs.attribute("id").value());
		pavs->ip = pj_str(strdup((char *)avs.attribute("ip").value()));
		pavs->port = atoi(avs.attribute("port").value());
		g_proxy_config.avs.push_back(pavs);
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

	RoomMgr mgr(g_proxy_config.local_ip,
		g_proxy_config.local_tcp_port,
		g_proxy_config.local_udp_port,
		g_proxy_config.thread_cnt);

	pj_str_t log_file_name = pj_str(const_cast<char *>("avsproxy.log"));
	status = mgr.Prepare(log_file_name);
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	status = mgr.Launch();
	RETURN_VAL_IF_FAIL(status == PJ_SUCCESS, status);

	PJ_LOG(5, ("AvsProxy","Proxy start........"));

	for (int i = 0; i < 10; ++i)
	{
		for(pj_uint32_t i = 0; i < g_proxy_config.avs.size(); ++ i)
		{
			request_to_avs_logout_t req_logout;
			req_logout.proxy_request_type = REQUEST_FROM_AVSPROXY_TO_AVS_LOGOUT;
			req_logout.proxy_id = 100;
			req_logout.room_id = 0;
			pj_ssize_t sndlen = sizeof(req_logout);
			status = mgr.SendRTPPacket(g_proxy_config.avs[i]->ip, g_proxy_config.avs[i]->port, RTP_EXPAND_PAYLOAD_TYPE, &req_logout, sndlen);
		}
	}

	console_main(mgr);

	return 1;
}
