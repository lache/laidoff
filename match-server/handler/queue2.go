package handler

import (
	"net"
	"log"
	"../user"
	"../convert"
)

func HandleQueue2(matchQueue chan<- user.UserAgent, buf []byte, conn net.Conn) {
	log.Printf("QUEUE2 received")
	recvPacket, err := convert.ParseQueue2(buf)
	if err != nil {
		log.Printf("HandleQueue2 fail: %v", err.Error())
		return
	}
	userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		// Queue connection
		matchQueue <- user.UserAgent{conn, *userDb}
		// Send reply
		queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
		conn.Write(queueOkBuf)
		log.Printf("Nickname '%v' queued", userDb.Nickname)
	}
}