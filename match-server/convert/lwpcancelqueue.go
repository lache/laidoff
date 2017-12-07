package convert

import (
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func NewLwpCancelQueueOk() *C.LWPCANCELQUEUEOK {
	return &C.LWPCANCELQUEUEOK{
		C.ushort(unsafe.Sizeof(C.LWPCANCELQUEUEOK{})),
		C.LPGP_LWPCANCELQUEUEOK,
	}
}
