package convert

import (
	"unsafe"
	"net"
	"../../shared-server"
)
// #include "../../src/puckgamepacket.h"
import "C"

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