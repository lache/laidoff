package handler

import (
	"bytes"
	"log"
	"time"
	"fmt"
	"net"
	"github.com/gasbank/laidoff/match-server/service"
	"github.com/gasbank/laidoff/match-server/convert"
)

func HandlePushToken(buf []byte, conn net.Conn, serviceList *service.List) {
	log.Printf("PUSHTOKEN received")
	// Parse
	recvPacket, err := convert.ParsePushToken(buf)
	if err != nil {
		log.Printf("HandlePushToken fail: %v", err.Error())
	}
	idBytes := convert.IdCuintToByteArray(recvPacket.Id)
	pushTokenBytes := convert.PushTokenBytes(recvPacket) // C.GoBytes(unsafe.Pointer(&recvPacket.Push_token), C.LW_PUSH_TOKEN_LENGTH)
	pushTokenLength := bytes.IndexByte(pushTokenBytes, 0)
	pushToken := string(pushTokenBytes[:pushTokenLength])
	log.Printf("Push token domain %v, token: %v, id: %v", recvPacket.Domain, pushToken, idBytes)
	pushResult := serviceList.Arith.RegisterPushToken(300*time.Millisecond, idBytes, int(recvPacket.Domain), pushToken)
	log.Printf("Push result: %v", pushResult)
	if pushResult == 1 {
		sysMsg := []byte(fmt.Sprintf("토큰 등록 완료! %v", pushToken))
		queueOkBuf := convert.Packet2Buf(convert.NewLwpSysMsg(sysMsg))
		conn.Write(queueOkBuf)
	}
}

