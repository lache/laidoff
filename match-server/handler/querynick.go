package handler

import (
	"os"
	"log"
	"net"
	"../nickdb"
	"../convert"
	"../user"
)

func HandleQueryNick(buf []byte, conn net.Conn, ndb *Nickdb.NickDb) {
	log.Printf("QUERYNICK received")
	recvPacket, err := convert.ParseQueryNick(buf)
	if err != nil {
		log.Printf("handleQueryNick fail: %v", err.Error())
		return
	}
	userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	if err != nil {
		if os.IsNotExist(err) {
			userDb, _, err = user.CreateNewUser(convert.IdCuintToByteArray(recvPacket.Id), Nickdb.PickRandomNick(ndb))
			if err != nil {
				log.Fatalf("load user db failed -> create new user failed")
			}
		} else {
			log.Fatalf("load user db failed: %v", err)
		}
	}

	log.Printf("User nick: %v", userDb.Nickname)
	// Send a reply
	nickname := userDb.Nickname
	nickBuf := convert.Packet2Buf(convert.NewLwpNick(nickname))
	_, err = conn.Write(nickBuf)
	if err != nil {
		log.Fatalf("LWPNICK send failed: %v", err.Error())
	}
}

