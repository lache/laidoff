package handler

import (
	"net"
	"log"
	"../user"
	"../convert"
	"../battle"
	"../config"
)

func HandleCancelQueue(conf config.ServerConfig, matchQueue chan<- user.UserAgent, buf []byte, conn net.Conn, ongoingBattleMap map[user.UserId]battle.Ok, battleService battle.Service, battleOkQueue chan<- battle.Ok) {
	log.Printf("CANCELQUEUE received")
	recvPacket, err := convert.ParseCancelQueue(buf)
	if err != nil {
		log.Printf("HandleCancelQueue fail: %v", err.Error())
		return
	}
	userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		// Check ongoing battle
		_, battleExists := ongoingBattleMap[userDb.Id]
		if battleExists {
			log.Printf("Nickname '%v' has the ongoing battle session. CANCELQUEUE?!", userDb.Nickname)
		}
		// Queue connection
		matchQueue <- user.UserAgent{conn, *userDb, true }
	}
}
