package battle

import (
	"log"
	"unsafe"
	"bytes"
	"encoding/binary"
	"../user"
	"../config"
	"../convert"
	"net"
)
// #include "../../src/puckgamepacket.h"
import "C"

type Ok struct {
	createBattleOk C.LWPCREATEBATTLEOK
	c1 user.UserAgent
	c2 user.UserAgent
}

func CreateBattleInstance(conf config.ServerConfig, c1 user.UserAgent, c2 user.UserAgent, battleOkQueue chan<- Ok) {
	// Connect to battle service
	tcpAddr, err := net.ResolveTCPAddr(conf.BattleServiceConnType, conf.BattleServiceHost+":"+conf.BattleServicePort)
	if err != nil {
		log.Fatalf("ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Fatalf("DialTCP error! - %v", err.Error())
	}
	// Send create battle request

	createBattleBuf := convert.Packet2Buf(convert.NewCreateBattle(
		c1.Db.Id,
		c2.Db.Id,
		c1.Db.Nickname,
		c2.Db.Nickname,
	))
	_, err = conn.Write(createBattleBuf)
	if err != nil {
		log.Fatalf("Send LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE failed")
	}
	// Wait for a reply
	createBattleOk := C.LWPCREATEBATTLEOK{}
	createBattleOkBuf := make([]byte, unsafe.Sizeof(C.LWPCREATEBATTLEOK{}))
	createBattleOkBufLen, err := conn.Read(createBattleOkBuf)
	if err != nil {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply failed")
		return
	}
	if createBattleOkBufLen != int(unsafe.Sizeof(C.LWPCREATEBATTLEOK{})) {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply size error")
		return
	}
	createBattleOkBufReader := bytes.NewReader(createBattleOkBuf)
	err = binary.Read(createBattleOkBufReader, binary.LittleEndian, &createBattleOk)
	if err != nil {
		log.Printf("binary.Read fail")
		return
	}
	if createBattleOk.Size == C.ushort(unsafe.Sizeof(C.LWPCREATEBATTLEOK{})) && createBattleOk.Type == C.LPGP_LWPCREATEBATTLEOK {
		// No error! so far ... proceed battle
		log.Printf("MATCH %v and %v matched successfully!", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
		battleOkQueue <- Ok{
			createBattleOk,
			c1,
			c2,
		}
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}

func OkWorker(conf config.ServerConfig, battleOkQueue <-chan Ok, ongoingBattleMap map[user.UserId]Ok) {
	for {
		battleOk := <-battleOkQueue
		ongoingBattleMap[battleOk.c1.Db.Id] = battleOk
		ongoingBattleMap[battleOk.c2.Db.Id] = battleOk
		WriteMatched2(conf, battleOk.c1.Conn, battleOk, battleOk.c1.Db.Id)
		WriteMatched2(conf, battleOk.c2.Conn, battleOk, battleOk.c2.Db.Id)
		//battleOk.c1.Conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, battleOk.createBattleOk.C1_token, 1, battleOk.c2.Db.Nickname))
		//battleOk.c2.Conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, battleOk.createBattleOk.C2_token, 2, battleOk.c1.Db.Nickname))
	}
}

func WriteMatched2(conf config.ServerConfig, conn net.Conn, battleOk Ok, id user.UserId) {
	if battleOk.c1.Db.Id == id {
		conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, battleOk.createBattleOk.C1_token, 1, battleOk.c2.Db.Nickname))
	} else if battleOk.c2.Db.Id == id {
		conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, battleOk.createBattleOk.C2_token, 2, battleOk.c1.Db.Nickname))
	}
}

func createMatched2Buf(conf config.ServerConfig, createBattleOk C.LWPCREATEBATTLEOK, token C.uint, playerNo C.int, targetNickname string) []byte {
	publicAddr, err := net.ResolveTCPAddr(conf.BattlePublicServiceConnType, conf.BattlePublicServiceHost+":"+conf.BattlePublicServicePort)
	if err != nil {
		log.Panicf("BattlePublicService conf parse error: %v", err.Error())
	}
	publicAddrIpv4 := publicAddr.IP.To4()

	return convert.Packet2Buf(convert.NewMatched2(
		publicAddr.Port,
		publicAddrIpv4,
		int(createBattleOk.Battle_id),
		uint(token),
		int(playerNo),
		targetNickname,
	))
}