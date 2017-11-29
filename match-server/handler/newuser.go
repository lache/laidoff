package handler

import (
	"net"
	"log"
	"../user"
	"../nickdb"
	"../convert"
)

func HandleNewUser(nickDb *Nickdb.NickDb, conn net.Conn) {
	log.Printf("NEWUSER received")
	uuid, uuidStr, err := user.NewUuid()
	if err != nil {
		log.Fatalf("new uuid failed: %v", err.Error())
	}
	log.Printf("  - New user guid: %v", uuidStr)
	newNick := Nickdb.PickRandomNick(nickDb)
	newUserDataBuf := convert.Packet2Buf(convert.NewNewUserData(uuid, newNick))
	// Write to disk
	var id user.UserId
	copy(id[:], uuid)
	_, _, err = user.CreateNewUser(id, newNick)
	if err != nil {
		log.Fatalf("CreateNewUser failed: %v", err.Error())
	}
	_, err = conn.Write(newUserDataBuf)
	if err != nil {
		log.Fatalf("NEWUSERDATA send failed: %v", err.Error())
	}
}
