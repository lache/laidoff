package handler

import (
	"os"
	"log"
	"net"
	"github.com/gasbank/laidoff/match-server/nickdb"
	"github.com/gasbank/laidoff/match-server/convert"
	"github.com/gasbank/laidoff/match-server/user"
)

func HandleQueryNick(buf []byte, conn net.Conn, ndb *nickdb.NickDb) {
	log.Printf("QUERYNICK received")
	recvPacket, err := convert.ParseQueryNick(buf)
	if err != nil {
		log.Printf("handleQueryNick fail: %v", err.Error())
		return
	}
	userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	if err != nil {
		if os.IsNotExist(err) {
			userDb, _, err = user.CreateNewUser(convert.IdCuintToByteArray(recvPacket.Id), nickdb.PickRandomNick(ndb))
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

