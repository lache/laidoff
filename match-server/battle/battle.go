package battle

import (
	"log"
	"bytes"
	"encoding/binary"
	"../user"
	"../config"
	"../convert"
	"net"
	"reflect"
	"errors"
	"unsafe"
)

type Ok struct {
	RemoveCache    bool
	BattleId       int
	createBattleOk convert.CreateBattleOk // C.LWPCREATEBATTLEOK
	c1             user.UserAgent
	c2             user.UserAgent
	RemoveUserId   user.UserId
}

type Service struct {
	Conf config.ServerConfig
}

func (sc *Service) Connection() (net.Conn, error) {
	tcpAddr, err := net.ResolveTCPAddr(sc.Conf.BattleServiceConnType, sc.Conf.BattleServiceHost+":"+sc.Conf.BattleServicePort)
	if err != nil {
		log.Fatalf("Battle service ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Printf("Battle service DialTCP error! - %v", err.Error())
	}
	return conn, err
}

func WaitForReply(connToBattle net.Conn, replyPacketRef interface{}, expectedReplySize uintptr, expectedReplyType int) error {
	replyBuf := make([]byte, expectedReplySize)
	replyBufLen, err := connToBattle.Read(replyBuf)
	if err != nil {
		log.Printf("Recv %v reply failed - %v", reflect.TypeOf(replyPacketRef).String(), err.Error())
		return err
	}
	if replyBufLen != int(expectedReplySize) {
		log.Printf("Recv %v reply size error - %v expected, got %v", reflect.TypeOf(replyPacketRef).String(), expectedReplySize, replyBufLen)
		return errors.New("read size not match")
	}
	replyBufReader := bytes.NewReader(replyBuf)
	err = binary.Read(replyBufReader, binary.LittleEndian, replyPacketRef)
	if err != nil {
		log.Printf("binary.Read fail - %v", err)
		return err
	}
	packetSize := int(binary.LittleEndian.Uint16(replyBuf[0:2]))
	packetType := int(binary.LittleEndian.Uint16(replyBuf[2:4]))
	if packetSize == int(expectedReplySize) && packetType == expectedReplyType {
		return nil
	}
	return errors.New("parsed size or type error")
}

func CreateBattleInstance(battleService Service, c1 user.UserAgent, c2 user.UserAgent, battleOkQueue chan<- Ok) {
	connToBattle, err := battleService.Connection()
	if err != nil {
		log.Printf("battleService error! %v", err.Error())
		SendRetryQueueLater(c1.Conn)
		SendRetryQueueLater(c2.Conn)
	} else {
		// Send create battle request
		createBattleBuf := convert.Packet2Buf(convert.NewCreateBattle(
			c1.Db.Id,
			c2.Db.Id,
			c1.Db.Nickname,
			c2.Db.Nickname,
		))
		_, err = connToBattle.Write(createBattleBuf)
		if err != nil {
			log.Fatalf("Send LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE failed")
		}
		// Wait for a reply
		createBattleOk, createBattleOkEnum := convert.NewLwpCreateBattleOk()
		err = WaitForReply(connToBattle, createBattleOk, unsafe.Sizeof(*createBattleOk), createBattleOkEnum)
		if err != nil {
			log.Printf("WaitForReply failed - %v", err.Error())
		} else {
			// No error! so far ... proceed battle
			log.Printf("MATCH %v and %v matched successfully!", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
			createBattleOkWrap := convert.CreateBattleOk{S: *createBattleOk}
			battleOkQueue <- Ok {
				false,
				int(createBattleOk.Battle_id),
				createBattleOkWrap,
				c1,
				c2,
				user.UserId{},
			}
		}
	}
}

func OkWorker(conf config.ServerConfig, battleOkQueue <-chan Ok, ongoingBattleMap map[user.UserId]Ok) {
	for {
		battleOk := <-battleOkQueue
		if battleOk.RemoveCache == false {
			log.Printf("OkWorker: received battle ok. Sending MATCHED2 to both clients")
			ongoingBattleMap[battleOk.c1.Db.Id] = battleOk
			ongoingBattleMap[battleOk.c2.Db.Id] = battleOk
			WriteMatched2(conf, battleOk.c1.Conn, battleOk, battleOk.c1.Db.Id)
			WriteMatched2(conf, battleOk.c2.Conn, battleOk, battleOk.c2.Db.Id)
		} else {
			log.Printf("OkWorker: received battle ok [remove]. Removing cache entry for user %v", battleOk.RemoveUserId)
			delete(ongoingBattleMap, battleOk.RemoveUserId)
		}
	}
}

func WriteMatched2(conf config.ServerConfig, conn net.Conn, battleOk Ok, id user.UserId) {
	if battleOk.c1.Db.Id == id {
		conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, uint32(battleOk.createBattleOk.S.C1_token), 1, battleOk.c2.Db.Nickname))
	} else if battleOk.c2.Db.Id == id {
		conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, uint32(battleOk.createBattleOk.S.C2_token), 2, battleOk.c1.Db.Nickname))
	}
}

func createMatched2Buf(conf config.ServerConfig, createBattleOk convert.CreateBattleOk, token uint32, playerNo int, targetNickname string) []byte {
	publicAddr, err := net.ResolveTCPAddr(conf.BattlePublicServiceConnType, conf.BattlePublicServiceHost+":"+conf.BattlePublicServicePort)
	if err != nil {
		log.Panicf("BattlePublicService conf parse error: %v", err.Error())
	}
	publicAddrIpv4 := publicAddr.IP.To4()

	return convert.Packet2Buf(convert.NewMatched2(
		publicAddr.Port,
		publicAddrIpv4,
		int(createBattleOk.S.Battle_id),
		uint(token),
		int(playerNo),
		targetNickname,
	))
}

func SendRetryQueue(conn net.Conn) {
	retryQueueBuf := convert.Packet2Buf(convert.NewLwpRetryQueue())
	_, retrySendErr := conn.Write(retryQueueBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry match packet to client", conn.RemoteAddr())
	}
}

func SendRetryQueueLater(conn net.Conn) {
	retryQueueLaterBuf := convert.Packet2Buf(convert.NewLwpRetryQueueLater())
	_, retrySendErr := conn.Write(retryQueueLaterBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry later match packet to client", conn.RemoteAddr())
	}
}