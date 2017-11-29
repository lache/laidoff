package convert

import (
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func NewLwpQueueOk() *C.LWPQUEUEOK {
	return &C.LWPQUEUEOK{
		C.ushort(unsafe.Sizeof(C.LWPQUEUEOK{})),
		C.LPGP_LWPQUEUEOK,
	}
}
