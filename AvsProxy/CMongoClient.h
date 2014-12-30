#ifndef __MyOwnIOService__CMongoClient__
#define __MyOwnIOService__CMongoClient__

#include <mutex>
#include "Com.h"
using std::mutex;
using std::lock_guard;


#include <mongoc/mongo.h>

#define DEF_CITY_LEN_OLD 20
#define DEF_CITY_LEN_NEW	64
#define DEF_CITY_LEN	DEF_CITY_LEN_OLD
#define DEF_PROV_LEN	DEF_CITY_LEN
#define DEF_STR_LEN	1024
#define MDB_TABLE_COL_RRTVMSS_ID_TINT		"rrtvmss_id"
#define MDB_TABLE_COL_RRTVMSS_IP_TSTR		"ip"
#define MDB_TABLE_COL_RRTVMSS_PORT_TINT		"port"
#define MDB_TABLE_COL_RRTVMSS_UPORT_TINT	"uport"
#define MDB_TABLE_COL_ROOMLIST_TARY			"room_list"
#define MDB_TABLE_COL_CITY_TSTR		"city_nm"
#define MDB_TABLE_COL_SIP_TBIGINT	"s_ip"
#define MDB_TABLE_COL_END_IP		"e_ip"
#define MDB_TABLE_COL_UID_TBIGINT	"uid"
#define MDB_TABLE_COL_PID_TINT		"pid"
#define MDB_TABLE_COL_IP_TBIGINT	"ip"
#define MDB_OP_LESSTHAN_AND_EQUAL	"$lte"
#define MDB_OP_QUERY				"$query"
#define MDB_OP_SORT					"$orderby"
#define MDB_OP_SET					"$set"
typedef struct _update_task
{
    enum    _type
    {
        _enNULLOP,
        _enUPDATE,
        _enINSERT,
        _enREMOVE,
        _enUPSERT,
		_enFIND
    };
	long long	_id;
	int			_muitype;
    bson		_mpcondition[1];
    bson		_mpoperation[1];
	long long	_mi64para;
	long long	_mi64para01;
    _update_task () : _id(0),
		_muitype(_enNULLOP),
		_mi64para(0)
    {
        memset( _mpcondition, 0, sizeof(bson) );
        memset( _mpoperation, 0, sizeof(bson) );
    }
    ~_update_task ()
    {
        bson_destroy( _mpcondition );
        bson_destroy( _mpoperation );
    }
} updatetask, &updatetaskref, *updatetaskptr;

class CMDBUpdaterInterface
{
public:
    virtual bool update ( updatetaskref ) = 0;
};


class CMDBInserterInterface
{
public:
    virtual bool insert ( updatetaskref )  = 0;
};


class CMDBRemoverInterface
{
public:
    virtual bool remove ( updatetaskref ) = 0;
};


class CMDBUpserterInterface
{
public:
    virtual bool upsert ( updatetaskref ) = 0;
};


class CMDBFinderInterface
{
public:
	virtual bool find ( updatetaskref ) = 0;
};

class CMongoClientBase : 
	public CMDBUpdaterInterface, 
	public CMDBInserterInterface, 
	public CMDBRemoverInterface, 
	public CMDBUpserterInterface, 
	public CMDBFinderInterface
{
public:
    CMongoClientBase ();
    virtual ~CMongoClientBase ();
    
public:
    bool	setopt ( int = 1000 );
    bool	startup ( const char* , unsigned int, const char* );
    void	shutdown ();
    bool	update ( updatetaskref );
    bool	remove ( updatetaskref );
    bool	insert ( updatetaskref );
    bool	upsert ( updatetaskref );
	bool	find(updatetaskref);
    int		checkconnection ();
	int		reconnect ();
    
public:
	bool	update ( bson*, bson* );
	bool	insert ( bson* );
	bool	remove ( bson* );
	bool	upsert ( bson*, bson* );
	bool	find(bson*, bson*){ return false; };
	bool	find_city ( bson*, bson*, char*, int, long long* );
	bool	find_provid_cityid_prov_city ( bson*, bson*, int*, int*, char*, int, char*, int, long long* );
	
private:
	char    _acnamespace[DEF_STR_LEN];
    mongo   _mmongo[1];

	enum	_mtconnection_state
	{
		enil,
		eConnected,
		eConnecting,
		eHungup
	};
	mutex							_mlock;
};

#endif
