#include "CMongoClient.h"

CMongoClientBase::CMongoClientBase ( ) 
{
	memset( _acnamespace, 0, sizeof( _acnamespace ) );
}

CMongoClientBase::~CMongoClientBase ()
{}

bool CMongoClientBase::setopt ( int ii32timeoutms )
{
    return (MONGO_OK == mongo_set_op_timeout( _mmongo, ii32timeoutms ) );
}

bool CMongoClientBase::startup ( const char * ipcip, unsigned int iuiport, const char* ipcns )
{
    bool booRet = false;
    int result = mongo_client( _mmongo, ipcip, iuiport );
    if( result != MONGO_OK )
    {
        switch( _mmongo->err )
        {
            case MONGO_CONN_NO_SOCKET:
                break;
            case MONGO_CONN_FAIL:
                break;
            case MONGO_CONN_ADDR_FAIL:
                break;
			case MONGO_CONN_NOT_MASTER:
				{
#if defined(PJ_WIN32) && PJ_WIN32!=0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0
					strncpy_s(_acnamespace, sizeof(_acnamespace)-1, ipcns, DEF_STR_LEN);
#else
					strncpy(_acnamespace, ipcns, sizeof(_acnamespace)-1);
#endif
				}
                break;
            default:
                break;
        }
    }
    else
    {
		strncpy_s(_acnamespace, ipcns, sizeof(_acnamespace)-1);
		booRet = true;
    }
    return booRet;
}

void CMongoClientBase::shutdown ()
{
	mongo_disconnect( _mmongo );
    mongo_destroy( _mmongo );
}

bool CMongoClientBase::update ( updatetaskref irtask )
{
    bool booRet = false;
	booRet	=	update( irtask._mpcondition, irtask._mpoperation );
	irtask._mi64para01	=	(long long)booRet;
    return booRet;
}

bool CMongoClientBase::insert ( updatetaskref irtask )
{
    bool booRet = false;
	booRet	=	insert( irtask._mpcondition );
	irtask._mi64para01	=	(long long)booRet;
    return booRet;
}

bool CMongoClientBase::remove ( updatetaskref irtask )
{
    bool booRet = false; 
	booRet = remove( irtask._mpcondition );
	irtask._mi64para01 = (long long)booRet;
    return booRet;
}

bool CMongoClientBase::upsert ( updatetaskref irtask )
{
	bool booRet = false;
	booRet = upsert( irtask._mpcondition, irtask._mpoperation );
	irtask._mi64para01 = (long long)booRet;
    return booRet;
}

bool CMongoClientBase::find ( updatetaskref irtask )
{
	bool booRet	= false;
	booRet = find( irtask._mpcondition, irtask._mpoperation );
	irtask._mi64para01 = (long long)booRet;
	return booRet;
}

bool CMongoClientBase::update ( bson* ircon, bson* irop )
{
    bool booRet = false;
	{
		lock_guard<mutex> lock(_mlock);
		if( mongo_update( _mmongo, _acnamespace, ircon, irop, MONGO_UPDATE_BASIC, 0 ) != MONGO_ERROR )
		{
			booRet = true;
		}
	}
    return booRet;
}

bool CMongoClientBase::insert ( bson* ircon )
{
    bool booRet = false;
	{
		lock_guard<mutex> lock(_mlock);
		if( mongo_insert( _mmongo, _acnamespace, ircon, 0 ) != MONGO_ERROR )
		{
			booRet = true;
		}
	}

    return booRet;
}

bool CMongoClientBase::remove ( bson* ircon )
{
    bool booRet = false;
	{
		lock_guard<mutex> lock(_mlock);
		if( mongo_remove( _mmongo, _acnamespace, ircon, 0 ) != MONGO_ERROR )
		{
			booRet = true;
		}
	}
    return booRet;
}

bool CMongoClientBase::upsert ( bson* ircon, bson* irop )
{
	bool booRet = false;
	{
		lock_guard<mutex> lock(_mlock);
		if( mongo_update( _mmongo, _acnamespace, ircon, irop, MONGO_UPDATE_UPSERT, 0 ) != MONGO_ERROR )
		{
			booRet = true;
		}
	}
    return booRet;
}

int CMongoClientBase::checkconnection ()
{
	int intRet = -1;
	{
		lock_guard<mutex> lock(_mlock);
		intRet = mongo_check_connection( _mmongo );
	}
	return intRet;
}

int CMongoClientBase::reconnect ()
{
	int intRet = -1;
	{
		lock_guard<mutex> lock(_mlock);
		intRet = mongo_reconnect( _mmongo );
	}
	return intRet;
}

