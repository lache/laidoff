package handler

import (
	"net"
	"log"
	"../user"
	"../convert"
	"../battle"
	"../config"
	"unsafe"
)
// #include "../../src/puckgamepacket.h"
import "C"

func HandleQueue2(conf config.ServerConfig, matchQueue chan<- user.UserAgent, buf []byte, conn net.Conn, ongoingBattleMap map[user.UserId]battle.Ok, battleService battle.Service, battleOkQueue chan<- battle.Ok) {
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
		battleOk, battleExists := ongoingBattleMap[userDb.Id]
		if battleExists {
			log.Printf("Nickname '%v' has the ongoing battle session. Check this battle still valid...", userDb.Nickname)
			connToBattle, err := battleService.Connection()
			if err != nil {
				log.Printf("Connection to battle service failed - %v", err.Error())
				log.Printf("Assume ongoing battle session is invalid")
				// Battle is not valid. Remove from cache
				battleOkQueue <- battle.Ok{
					RemoveCache:    true,
					RemoveUserId:   userDb.Id,
				}
				// And then fallback to queuing this user
			} else {
				checkBattleValidBuf := convert.Packet2Buf(convert.NewCheckBattleValid(battleOk.BattleId))
				connToBattle.Write(checkBattleValidBuf)
				battleValid := &C.LWPBATTLEVALID{}
				err = battle.WaitForReply(connToBattle, battleValid, unsafe.Sizeof(*battleValid), int(C.LPGP_LWPBATTLEVALID))
				if err != nil {
					log.Printf("WaitForReply failed - %v", err.Error())
				} else {
					if battleValid.Valid == 1 {
						// [1] QUEUEOK
						queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
						conn.Write(queueOkBuf)
						// [2] MAYBEMATCHED
						maybeMatchedBuf := convert.Packet2Buf(convert.NewLwpMaybeMatched())
						conn.Write(maybeMatchedBuf)
						// [3] MATCHED2
						battle.WriteMatched2(conf, conn, battleOk, userDb.Id)
						// Should exit this function here
						return
					} else {
						// Battle is not valid. Remove from cache
						battleOkQueue <- battle.Ok{
							RemoveCache:    true,
							RemoveUserId:   userDb.Id,
						}
						// And then fallback to queuing this user
					}
				}
			}
		}
		// Queue connection
		matchQueue <- user.UserAgent{conn, *userDb, false }
		// Send reply
		queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
		conn.Write(queueOkBuf)
		log.Printf("Nickname '%v' queued", userDb.Nickname)
	}
}
