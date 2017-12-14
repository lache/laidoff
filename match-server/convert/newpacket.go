package convert

import (
	"unsafe"
	"net"
	"../../shared-server"
	"encoding/binary"
)
// #include "../../src/puckgamepacket.h"
import "C"

const (
	LPGP_LWPQUEUE2 = C.LPGP_LWPQUEUE2
	LPGP_LWPCANCELQUEUE = C.LPGP_LWPCANCELQUEUE
	LPGP_LWPSUDDENDEATH = C.LPGP_LWPSUDDENDEATH
	LPGP_LWPNEWUSER = C.LPGP_LWPNEWUSER
	LPGP_LWPQUERYNICK = C.LPGP_LWPQUERYNICK
	LPGP_LWPPUSHTOKEN = C.LPGP_LWPPUSHTOKEN
	LPGP_LWPGETLEADERBOARD = C.LPGP_LWPGETLEADERBOARD
	LPGP_LWPSETNICKNAME = C.LPGP_LWPSETNICKNAME
)

type CreateBattleOk struct {
	S C.LWPCREATEBATTLEOK
}

func NewLwpBattleValid() (*C.LWPBATTLEVALID, int) {
	return &C.LWPBATTLEVALID{}, int(C.LPGP_LWPBATTLEVALID)
}

func NewLwpCreateBattleOk() (*C.LWPCREATEBATTLEOK, int) {
	return &C.LWPCREATEBATTLEOK{}, int(C.LPGP_LWPCREATEBATTLEOK)
}

func NewLwpRetryQueueLater() *C.LWPRETRYQUEUELATER {
	return &C.LWPRETRYQUEUELATER{
		C.ushort(unsafe.Sizeof(C.LWPRETRYQUEUELATER{})),
		C.LPGP_LWPRETRYQUEUELATER,
	}
}

func NewLwpRetryQueue() *C.LWPRETRYQUEUE {
	return &C.LWPRETRYQUEUE{
		C.ushort(unsafe.Sizeof(C.LWPRETRYQUEUE{})),
		C.LPGP_LWPRETRYQUEUE,
	}
}

func NewLwpCancelQueueOk() *C.LWPCANCELQUEUEOK {
	return &C.LWPCANCELQUEUEOK{
		C.ushort(unsafe.Sizeof(C.LWPCANCELQUEUEOK{})),
		C.LPGP_LWPCANCELQUEUEOK,
	}
}
func NewLwpMaybeMatched() *C.LWPMAYBEMATCHED {
	return &C.LWPMAYBEMATCHED{
		C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
		C.LPGP_LWPMAYBEMATCHED,
	}
}
func NewLwpNick(nick string) *C.LWPNICK {
	cNewNickBytes := NicknameToCArray(nick)
	return &C.LWPNICK{
		C.ushort(unsafe.Sizeof(C.LWPNICK{})),
		C.LPGP_LWPNICK,
		cNewNickBytes,
	}
}
func NewLwpQueueOk() *C.LWPQUEUEOK {
	return &C.LWPQUEUEOK{
		C.ushort(unsafe.Sizeof(C.LWPQUEUEOK{})),
		C.LPGP_LWPQUEUEOK,
	}
}
func NewLwpSetNicknameResult(request *C.LWPSETNICKNAME) *C.LWPSETNICKNAMERESULT {
	return &C.LWPSETNICKNAMERESULT{
		C.ushort(unsafe.Sizeof(C.LWPSETNICKNAMERESULT{})),
		C.LPGP_LWPSETNICKNAMERESULT,
		request.Id,
		request.Nickname,
	}
}

func NewMatched2(port int, ipv4 net.IP, battleId int, token uint, playerNo int, targetNickname string) *C.LWPMATCHED2 {
	return &C.LWPMATCHED2{
		C.ushort(unsafe.Sizeof(C.LWPMATCHED2{})),
		C.LPGP_LWPMATCHED2,
		C.ushort(port), // createBattleOk.Port
		C.ushort(0),               // padding
		[4]C.uchar{C.uchar(ipv4[0]), C.uchar(ipv4[1]), C.uchar(ipv4[2]), C.uchar(ipv4[3]),},
		C.int(battleId),
		C.uint(token),
		C.int(playerNo),
		NicknameToCArray(targetNickname),
	}
}

func NewLeaderboard(leaderboardReply *shared_server.LeaderboardReply) *C.LWPLEADERBOARD {
	var nicknameList [C.LW_LEADERBOARD_ITEMS_IN_PAGE][C.LW_NICKNAME_MAX_LEN]C.char
	var scoreList [C.LW_LEADERBOARD_ITEMS_IN_PAGE]C.int
	for i, item := range leaderboardReply.Items {
		ConvertGoStringToCCharArray(&item.Nickname, &nicknameList[i])
		scoreList[i] = C.int(item.Score)
	}
	return &C.LWPLEADERBOARD{
		C.ushort(unsafe.Sizeof(C.LWPLEADERBOARD{})),
		C.LPGP_LWPLEADERBOARD,
		C.int(len(leaderboardReply.Items)),
		C.int(leaderboardReply.FirstItemRank),
		C.int(leaderboardReply.FirstItemTieCount),
		nicknameList,
		scoreList,
	}
}
func NewNewUserData(uuid []byte, newNick string) *C.LWPNEWUSERDATA {
	cNewNickBytes := NicknameToCArray(newNick)
	return &C.LWPNEWUSERDATA{
		C.ushort(unsafe.Sizeof(C.LWPNEWUSERDATA{})),
		C.LPGP_LWPNEWUSERDATA,
		[4]C.uint{C.uint(binary.BigEndian.Uint32(uuid[0:4])), C.uint(binary.BigEndian.Uint32(uuid[4:8])), C.uint(binary.BigEndian.Uint32(uuid[8:12])), C.uint(binary.BigEndian.Uint32(uuid[12:16]))},
		cNewNickBytes,
	}
}
func NewSysMsg(sysMsg []byte) *C.LWPSYSMSG {
	return &C.LWPSYSMSG{
		C.ushort(unsafe.Sizeof(C.LWPSYSMSG{})),
		C.LPGP_LWPSYSMSG,
		ByteArrayToCcharArray256(sysMsg),
	}
}

func PushTokenBytes(recvPacket *C.LWPPUSHTOKEN) []byte {
	return C.GoBytes(unsafe.Pointer(&recvPacket.Push_token), C.LW_PUSH_TOKEN_LENGTH)
}