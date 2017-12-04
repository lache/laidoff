package convert

import (
	"encoding/binary"
	"fmt"
	"../user"
	"unsafe"
	"bytes"
)
// #include "../../src/puckgamepacket.h"
import "C"

func Int2ByteArray(v C.int) [4]byte {
	var byteArray [4]byte
	binary.LittleEndian.PutUint32(byteArray[0:], uint32(v))
	return byteArray
}

func ConvertGoStringToCCharArray(strIn *string, strOut *[C.LW_NICKNAME_MAX_LEN]C.char) {
	for i, b := range []byte(*strIn) {
		(*strOut)[i] = C.char(b)
	}
}

func ConvertCCharArrayToGoString(strIn *[C.LW_NICKNAME_MAX_LEN]C.char, strOut *string) {
	bytes := make([]byte, len(strIn))
	lastIndex := 0
	for i, b := range strIn {
		bytes[i] = byte(b)
		if bytes[i] == 0 {
			lastIndex = i
			break
		}
	}
	*strOut = string(bytes[:lastIndex])
}

func ByteArrayToCcharArray256(b []byte) [256]C.char {
	var ret [256]C.char
	bLen := len(b)
	if bLen > 255 {
		bLen = 255
	}
	for i := 0; i < bLen; i++ {
		ret[i] = C.char(b[i])
	}
	return ret
}

func UserIdToCuint(id user.UserId) [4]C.uint {
	var b [4]C.uint
	b[0] = C.uint(binary.BigEndian.Uint32(id[0:4]))
	b[1] = C.uint(binary.BigEndian.Uint32(id[4:8]))
	b[2] = C.uint(binary.BigEndian.Uint32(id[8:12]))
	b[3] = C.uint(binary.BigEndian.Uint32(id[12:16]))
	return b
}

func IdCuintToByteArray(id [4]C.uint) user.UserId {
	var b user.UserId
	binary.BigEndian.PutUint32(b[0:], uint32(id[0]))
	binary.BigEndian.PutUint32(b[4:], uint32(id[1]))
	binary.BigEndian.PutUint32(b[8:], uint32(id[2]))
	binary.BigEndian.PutUint32(b[12:], uint32(id[3]))
	return b
}

func NicknameToCArray(nickname string) [C.LW_NICKNAME_MAX_LEN]C.char {
	var nicknameCchar [C.LW_NICKNAME_MAX_LEN]C.char
	for i, v := range []byte(nickname) {
		if i >= C.LW_NICKNAME_MAX_LEN {
			break
		}
		nicknameCchar[i] = C.char(v)
	}
	nicknameCchar[C.LW_NICKNAME_MAX_LEN-1] = 0
	return nicknameCchar
}

func IdArrayToString(id [4]C.uint) string {
	return fmt.Sprintf("%08x-%08x-%08x-%08x", id[0], id[1], id[2], id[3])
}

func NewCreateBattle(id1, id2 user.UserId, nickname1, nickname2 string) *C.LWPCREATEBATTLE {
	var c1Nickname [C.LW_NICKNAME_MAX_LEN]C.char
	var c2Nickname [C.LW_NICKNAME_MAX_LEN]C.char
	ConvertGoStringToCCharArray(&nickname1, &c1Nickname)
	ConvertGoStringToCCharArray(&nickname2, &c2Nickname)
	return &C.LWPCREATEBATTLE{
		C.ushort(unsafe.Sizeof(C.LWPCREATEBATTLE{})),
		C.LPGP_LWPCREATEBATTLE,
		UserIdToCuint(id1),
		UserIdToCuint(id2),
		c1Nickname,
		c2Nickname,
	}
}

func NewCheckBattleValid(battleId int) *C.LWPCHECKBATTLEVALID {
	return &C.LWPCHECKBATTLEVALID{
		C.ushort(unsafe.Sizeof(C.LWPCHECKBATTLEVALID{})),
		C.LPGP_LWPCHECKBATTLEVALID,
		C.int(battleId),
	}
}

func Packet2Buf(packet interface{}) []byte {
	buf := &bytes.Buffer{}
	binary.Write(buf, binary.LittleEndian, packet)
	return buf.Bytes()
}
