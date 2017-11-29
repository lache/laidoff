package convert

import (
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func NewSysMsg(sysMsg []byte) *C.LWPSYSMSG {
	return &C.LWPSYSMSG{
		C.ushort(unsafe.Sizeof(C.LWPSYSMSG{})),
		C.LPGP_LWPSYSMSG,
		ByteArrayToCcharArray256(sysMsg),
	}
}
