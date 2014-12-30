#ifndef __AVS_PROXY_ROOM__
#define __AVS_PROXY_ROOM__

#include <vector>
#include <mutex>
#include <map>
#include <set>

#include "Com.h"
#include "RoomUser.h"
#include "Termination.h"

using std::set;
using std::map;
using std::vector;
using std::mutex;
using std::lock_guard;

typedef set<Termination *> watcher_set_t;  /**< Termination may be constant!! */
typedef map<pj_int64_t, RoomUser *> user_map_t;
class Room /* same as AVS */
{
public:
	enum _stat {
		_enUnavalible,
		_enBuilding,
		_enAvaliable
	};
public:
	Room( pj_uint32_t, const pj_str_t &, pj_uint16_t, pj_sock_t &, pjmedia_rtp_session &, mutex &);
	~Room();

	pj_status_t Prepare();
	pj_status_t Launch();
	void        Destory();
	void        OnConsole(stringstream &output);
	user_map_t &GetUsers();
	/**< 添加观察者 房间用户信息的任何变更都应该通知所有观察者 */
	/**< just like link a room */
	pj_status_t OnWatched(Termination *termination);

	/**< 取消观察者 */
	/**< just like unlink a room */
	pj_status_t OnUnwatch(Termination *termination);

	pj_status_t OnLinkUser(pj_int64_t, pj_uint16_t, const pj_str_t &, pj_uint16_t, pj_uint8_t, pj_bool_t &);
	pj_status_t OnUnlinkUser(pj_int64_t, pj_uint16_t, pj_bool_t &);

	/**< 添加房间上麦用户ID, SSRC */
	pj_status_t OnAddUser(pj_int64_t user_id, pj_uint32_t mic_id, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);

	/**< 添加房间上麦用户ID */
	pj_status_t OnAddUser(pj_int64_t user_id, pj_uint32_t mic_id);

	/**< 修改房间上麦用户的SSRC */
	pj_status_t OnModUser(pj_int64_t user_id, pj_uint32_t audio_ssrc, pj_uint32_t video_ssrc);

	/**< 房间用户下麦 */
	pj_status_t OnDelUser(pj_int64_t);

	pj_status_t OnRxLogin(pj_bool_t);
	pj_status_t OnRxKeepAlive();
	void        OnRxRtp(pj_int64_t, pj_uint8_t, const vector<pj_uint8_t> &);
	RoomUser *  GetUser(pj_int64_t);
	pj_status_t SendRTPPacket(const void *, pj_ssize_t *);

	//Added by Dan.
	size_t		GetCurRoomAudiencesNb();
	bool		GetRoomInfo(room_info_t&);
	_stat		GetCurRoomState();
	void		SetRoomState(_stat);
private:
	pj_str_t      room_ip_;
	pj_uint16_t   room_port_;
	mutex         room_lock_;
	room_id_t     room_id_;
	mutex         watchers_lock_;
	watcher_set_t watchers_;
	user_map_t    online_users_;    // 此房间的所有上麦用户信息
	pj_sock_t           &ref_udp_sock_;
	pjmedia_rtp_session &ref_rtp_session_;
	mutex               &ref_rtp_sess_lock_;
	_stat				_room_stat;
};

#endif
