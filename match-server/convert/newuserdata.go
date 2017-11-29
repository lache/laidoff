package convert

import (
	"unsafe"
	"encoding/binary"
)
// #include "../../src/puckgamepacket.h"
import "C"
func NewNewUserData(uuid []byte, newNick string) *C.LWPNEWUSERDATA {
	cNewNickBytes := NicknameToCArray(newNick)
	return &C.LWPNEWUSERDATA{
		C.ushort(unsafe.Sizeof(C.LWPNEWUSERDATA{})),
		C.LPGP_LWPNEWUSERDATA,
		[4]C.uint{C.uint(binary.BigEndian.Uint32(uuid[0:4])), C.uint(binary.BigEndian.Uint32(uuid[4:8])), C.uint(binary.BigEndian.Uint32(uuid[8:12])), C.uint(binary.BigEndian.Uint32(uuid[12:16]))},
		cNewNickBytes,
	}
}
