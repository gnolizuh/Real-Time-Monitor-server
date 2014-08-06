#include "UsersInfoScene.h"

UsersInfoParameter::UsersInfoParameter(const pj_uint8_t *storage)
: UdpParameter(storage)
{
	const pj_uint8_t *tmp_storage = storage + sizeof(UdpParameter);
	user_count_ =  *(pj_uint8_t *)tmp_storage; tmp_storage += sizeof(pj_uint8_t); // Instead by Macro.
	users_.resize(user_count_);

	for(pj_uint8_t idx = 0; idx < user_count_; ++ idx)
	{
		users_[idx].user_id = (pj_int64_t)pj_ntohll(*(pj_int64_t *)tmp_storage); tmp_storage += sizeof(pj_int64_t);
		users_[idx].audio_ssrc = (pj_uint32_t)pj_ntohl(*(pj_uint32_t *)tmp_storage); tmp_storage += sizeof(pj_uint32_t);
		users_[idx].video_ssrc = (pj_uint32_t)pj_ntohl(*(pj_uint32_t *)tmp_storage); tmp_storage += sizeof(pj_uint32_t);
	}
}

void UsersInfoScene::Maintain(UdpParameter *parameter, Room *room)
{
	RETURN_IF_FAIL( parameter != nullptr && room != nullptr );

	UsersInfoParameter *param = reinterpret_cast<UsersInfoParameter *>(parameter);
	for( pj_uint8_t idx = 0; idx < param->user_count_; ++ idx )
	{
		room->Add(param->users_[idx].user_id,
			param->users_[idx].audio_ssrc,
			param->users_[idx].video_ssrc);
	}
}
