package convert

import (
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func NewLwpNick(nick string) *C.LWPNICK {
	cNewNickBytes := NicknameToCArray(nick)
	return &C.LWPNICK{
		C.ushort(unsafe.Sizeof(C.LWPNICK{})),
		C.LPGP_LWPNICK,
		cNewNickBytes,
	}
}
