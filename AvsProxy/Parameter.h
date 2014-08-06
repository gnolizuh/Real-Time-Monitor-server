#ifndef __AVS_PROXY_PARAMETER__
#define __AVS_PROXY_PARAMETER__

#include "Com.h"

class Parameter
{
public:
	Parameter(const pj_uint8_t *storage)
	{
		const pj_uint16_t *u16_storage = (const pj_uint16_t *)(storage);
		length_    = ntohs(*(u16_storage ++));
		type_      = ntohs(*(u16_storage ++));
		proxy_id_  = ntohs(*(u16_storage ++));
		client_id_ = ntohs(*(u16_storage));
	}

	pj_uint16_t length_;
	pj_uint16_t type_;
	pj_uint16_t proxy_id_;
	pj_uint16_t client_id_;
};

#endif
