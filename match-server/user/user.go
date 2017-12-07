package user

import (
	"os"
	"log"
	"encoding/gob"
	"time"
	"fmt"
	"net"
	"io"
	cryptorand "crypto/rand"
)

type UserId [16]byte

type UserDb struct {
	Id       UserId
	Created  time.Time
	Nickname string
}

type UserAgent struct {
	Conn        net.Conn
	Db          UserDb
	CancelQueue bool
}

func NewUuid() ([]byte, string, error) {
	uuid := make([]byte, 16)

	n, err := io.ReadFull(cryptorand.Reader, uuid)
	if n != len(uuid) || err != nil {
		return nil, "", err
	}
	// variant bits; see section 4.1.1
	uuid[8] = uuid[8]&^0xc0 | 0x80
	// version 4 (pseudo-random); see section 4.1.3
	uuid[6] = uuid[6]&^0xf0 | 0x40
	return uuid, fmt.Sprintf("%08x-%08x-%08x-%08x", uuid[0:4], uuid[4:8], uuid[8:12], uuid[12:16]), nil
}

func IdByteArrayToString(id UserId) string {
	return fmt.Sprintf("%08x-%08x-%08x-%08x", id[0:4], id[4:8], id[8:12], id[12:16])
}

func LoadUserDb(id UserId) (*UserDb, error) {
	uuidStr := IdByteArrayToString(id)
	userDbFile, err := os.Open("db/" + uuidStr)
	if err != nil {
		log.Printf("disk open failed: %v", err.Error())
		return nil, err
	} else {
		defer userDbFile.Close()
		decoder := gob.NewDecoder(userDbFile)
		userDb := &UserDb{}
		decoder.Decode(userDb)
		return userDb, nil
	}
}

func CreateNewUser(uuid UserId, nickname string) (*UserDb, *os.File, error) {
	userDb := &UserDb{
		uuid,
		time.Now(),
		nickname,
	}
	uuidStr := IdByteArrayToString(uuid)
	userDbFile, err := os.Create("db/" + uuidStr)
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return nil, nil, err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	return userDb, userDbFile, nil
}

func WriteUserDb(userDb *UserDb) error {
	userDbFile, err := os.Create("db/" + IdByteArrayToString(userDb.Id))
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	return nil
}
