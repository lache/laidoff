package user

import (
	"time"
	"fmt"
	"net"
	"io"
	"crypto/rand"
)

type Id [16]byte
type LeaseId [16]byte

type Db struct {
	Id       Id
	Created  time.Time
	Nickname string
}

type LeaseDb struct {
	LeaseId LeaseId
	Db      Db
}

type Agent struct {
	Conn        net.Conn
	Db          Db
	CancelQueue bool
}

func NewUuid() ([]byte, string, error) {
	uuid := make([]byte, 16)

	n, err := io.ReadFull(rand.Reader, uuid)
	if n != len(uuid) || err != nil {
		return nil, "", err
	}
	// variant bits; see section 4.1.1
	uuid[8] = uuid[8]&^0xc0 | 0x80
	// version 4 (pseudo-random); see section 4.1.3
	uuid[6] = uuid[6]&^0xf0 | 0x40
	return uuid, fmt.Sprintf("%08x-%08x-%08x-%08x", uuid[0:4], uuid[4:8], uuid[8:12], uuid[12:16]), nil
}

func IdByteArrayToString(id Id) string {
	return fmt.Sprintf("%08x-%08x-%08x-%08x", id[0:4], id[4:8], id[8:12], id[12:16])
}
