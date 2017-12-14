package handler

import (
	"log"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/match-server/convert"
	"net"
	"github.com/gasbank/laidoff/match-server/rpchelper"
)

func HandleSetNickname(buf []byte, conn net.Conn, dbService *rpchelper.Context) {
	log.Printf("SETNICKNAME received")
	// Parse
	recvPacket, err := convert.ParseSetNickname(buf)
	if err != nil {
		log.Printf("HandleSetNickname fail: %v", err.Error())
		return
	}
	//userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	var userLeaseDb user.LeaseDb
	err = dbService.Call("Lease", convert.IdCuintToByteArray(recvPacket.Id), &userLeaseDb)
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		var newNickname string
		convert.CCharArrayToGoString(&recvPacket.Nickname, &newNickname)
		oldNickname := userLeaseDb.Db.Nickname
		userLeaseDb.Db.Nickname = newNickname
		//user.WriteUserDb(userDb)
		var writeReply int
		err = dbService.Call("Write", &userLeaseDb, &writeReply)
		if err != nil {
			log.Printf("DB service Write failed: %v", err)
		} else {
			queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket))
			conn.Write(queueOkBuf)
			log.Printf("Nickname '%v' changed to '%v'", oldNickname, userLeaseDb.Db.Nickname)
		}
	}
}
