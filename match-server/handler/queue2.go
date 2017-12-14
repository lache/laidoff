package handler

import (
	"net"
	"log"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/match-server/convert"
	"github.com/gasbank/laidoff/match-server/battle"
	"github.com/gasbank/laidoff/match-server/config"
	"unsafe"
	"github.com/gasbank/laidoff/match-server/rpchelper"
)

func HandleQueue2(conf config.ServerConfig, matchQueue chan<- user.Agent, buf []byte, conn net.Conn, ongoingBattleMap map[user.Id]battle.Ok, battleService battle.Service, battleOkQueue chan<- battle.Ok, dbService *rpchelper.Context) {
	log.Printf("QUEUE2 received")
	recvPacket, err := convert.ParseQueue2(buf)
	if err != nil {
		log.Printf("HandleQueue2 fail: %v", err.Error())
		return
	}
	var userDb user.Db
	err = dbService.Call("Get", convert.IdCuintToByteArray(recvPacket.Id), &userDb)
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
				battleValid, battleValidEnum := convert.NewLwpBattleValid()
				err = battle.WaitForReply(connToBattle, battleValid, unsafe.Sizeof(*battleValid), battleValidEnum)
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
		matchQueue <- user.Agent{conn, userDb, false }
		// Send reply
		queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
		conn.Write(queueOkBuf)
		log.Printf("Nickname '%v' queued", userDb.Nickname)
	}
}
