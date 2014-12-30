#include "RecvAVSStatusInfo.h"
#include "RoomMgr.h"
CRecvAVSStatusParameter::CRecvAVSStatusParameter(const pj_uint8_t *storage, pj_uint16_t storage_len)
:UdpParameter(storage, storage_len)
{
	pj_ntoh_assign(storage, storage_len, mu32avs_id );
	pj_ntoh_assign(storage, storage_len, mu32avs_ip );
	pj_ntoh_assign(storage, storage_len, mu16avs_port );
}

CRecvAVSStatusInfo::CRecvAVSStatusInfo()
{
}


CRecvAVSStatusInfo::~CRecvAVSStatusInfo()
{
}

scene_opt_t CRecvAVSStatusInfo::Maintain(shared_ptr<UdpParameter> ptr_udp_param, RoomMgr *roommgr)
{
	CRecvAVSStatusParameter *param = reinterpret_cast<CRecvAVSStatusParameter *>(ptr_udp_param.get());
	roommgr->UpdateAvsStatusInfo(param->room_id_, param->mu32avs_ip, param->mu16avs_port,(pj_uint64_t) GetTickCount64());
	return SCENE_OPT_NONE;
}
