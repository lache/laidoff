package main

import (
	"log"
	"net"
	"./helpers"
	"encoding/binary"
	"bytes"
	"time"
)

// #include "../src/puckgamepacket.h"
import "C"

const (
	ServiceName = "reward"
	ServiceAddr = ":10290"
	RankServiceAddr = ":20172"
)

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Printf("Greetings from %v service", ServiceName)
	// Connect to rank service
	rank := helpers.NewRankClient(RankServiceAddr)
	// Start listening
	l, err := net.Listen("tcp", ServiceAddr)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Printf("Listening %v for %v service...", ServiceAddr, ServiceName)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conn, rank)
		}
	}
}
func handleRequest(conn net.Conn, rank *helpers.RankClient) {
	log.Printf("Accepting from %v", conn.RemoteAddr())
	for {
		buf := make([]byte, 1024)
		//conn.SetReadDeadline(time.Now().Add(10000 * time.Millisecond))
		readLen, err := conn.Read(buf)
		if readLen == 0 {
			log.Printf("%v: readLen is zero.", conn.RemoteAddr())
			break
		}
		if err != nil {
			log.Printf("%v: Error reading: %v", conn.RemoteAddr(), err.Error())
		}
		log.Printf("%v: Packet received (readLen=%v)", conn.RemoteAddr(), readLen)
		packetSize := binary.LittleEndian.Uint16(buf)
		packetType := binary.LittleEndian.Uint16(buf[2:])
		log.Printf("  Size %v", packetSize)
		log.Printf("  Type %v", packetType)
		switch packetType {
		case C.LPGP_LWPBATTLERESULT:
			handleBattleResult(buf, rank)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}

func convertCCharArrayToGoString(strIn *[C.LW_NICKNAME_MAX_LEN]C.char, strOut *string) {
	buf := make([]byte, len(strIn))
	for i, b := range strIn {
		buf[i] = byte(b)
	}
	*strOut = string(buf)
}

func handleBattleResult(buf []byte, rank *helpers.RankClient) {
	log.Printf("BATTLERESULT received")
	// Parse
	bufReader := bytes.NewReader(buf)
	recvPacket := C.LWPBATTLERESULT{}
	err := binary.Read(bufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	id1 := IdCuintToByteArray(recvPacket.Id1)
	id2 := IdCuintToByteArray(recvPacket.Id2)
	var nickname1 string
	var nickname2 string
	convertCCharArrayToGoString(&recvPacket.Nickname1, &nickname1)
	convertCCharArrayToGoString(&recvPacket.Nickname2, &nickname2)
	winner := int(recvPacket.Winner)
	log.Printf("Battle result received; id1=%v, id2=%v, winner=%v", id1, id2, winner)
	backoff := 300 * time.Millisecond
	newScore1 := 0
	newScore2 := 0
	oldScore1 := rank.Get(backoff, id1)
	oldScore2 := rank.Get(backoff, id2)
	if oldScore1 != nil {
		newScore1 = oldScore1.Score + 1
		if winner == 1 {
			newScore1++
		}
	} else {
		newScore1 = 1
	}
	if oldScore2 != nil {
		newScore2 = oldScore2.Score + 1
		if winner == 2 {
			newScore2++
		}
	} else {
		newScore2 = 1
	}
	rank.Set(backoff, id1, newScore1, nickname1)
	rank.Set(backoff, id2, newScore2, nickname2)
}

type UserId [16]byte

func IdCuintToByteArray(id [4]C.uint) UserId {
	var b UserId
	binary.BigEndian.PutUint32(b[0:], uint32(id[0]))
	binary.BigEndian.PutUint32(b[4:], uint32(id[1]))
	binary.BigEndian.PutUint32(b[8:], uint32(id[2]))
	binary.BigEndian.PutUint32(b[12:], uint32(id[3]))
	return b
}
