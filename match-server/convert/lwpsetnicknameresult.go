package convert

import (
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func NewLwpSetNicknameResult(request *C.LWPSETNICKNAME) *C.LWPSETNICKNAMERESULT {
	return &C.LWPSETNICKNAMERESULT{
		C.ushort(unsafe.Sizeof(C.LWPSETNICKNAMERESULT{})),
		C.LPGP_LWPSETNICKNAMERESULT,
		request.Id,
		request.Nickname,
	}
}
