package handler

import (
	"log"
	"../user"
	"../convert"
	"net"
)

func HandleSetNickname(buf []byte, conn net.Conn) {
	log.Printf("SETNICKNAME received")
	// Parse
	recvPacket, err := convert.ParseSetNickname(buf)
	if err != nil {
		log.Printf("HandleSetNickname fail: %v", err.Error())
		return
	}
	userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		var newNickname string
		convert.CCharArrayToGoString(&recvPacket.Nickname, &newNickname)
		oldNickname := userDb.Nickname
		userDb.Nickname = newNickname
		user.WriteUserDb(userDb)

		queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket))
		conn.Write(queueOkBuf)
		log.Printf("Nickname '%v' changed to '%v'", oldNickname, userDb.Nickname)
	}
}
