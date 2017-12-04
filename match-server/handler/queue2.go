package handler

import (
	"net"
	"log"
	"../user"
	"../convert"
	"../battle"
	"../config"
)

func HandleQueue2(conf config.ServerConfig, matchQueue chan<- user.UserAgent, buf []byte, conn net.Conn, ongogingBattleMap map[user.UserId]battle.Ok) {
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
		// Check ongoing battle
		battleOk, battleExists := ongogingBattleMap[userDb.Id]
		if battleExists {
			log.Printf("Nickname '%v' has the ongoing battle session. Trying to resume battle...", userDb.Nickname)
			// [1] QUEUEOK
			queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
			conn.Write(queueOkBuf)
			// [2] MAYBEMATCHED
			maybeMatchedBuf := convert.Packet2Buf(convert.NewLwpMaybeMatched())
			conn.Write(maybeMatchedBuf)
			// [3] MATCHED2
			battle.WriteMatched2(conf, conn, battleOk, userDb.Id)
		} else {
			// Queue connection
			matchQueue <- user.UserAgent{conn, *userDb}
			// Send reply
			queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
			conn.Write(queueOkBuf)
			log.Printf("Nickname '%v' queued", userDb.Nickname)
		}
	}
}
