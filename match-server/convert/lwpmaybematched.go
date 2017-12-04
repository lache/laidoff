package convert

import (
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func NewLwpMaybeMatched() *C.LWPMAYBEMATCHED {
	return &C.LWPMAYBEMATCHED{
		C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
		C.LPGP_LWPMAYBEMATCHED,
	}
}
