package main

import (
	"log"
	"net"
	"github.com/gasbank/laidoff/reward-server/helpers"
	"github.com/gasbank/laidoff/match-server/convert"
	"encoding/binary"
	"bytes"
	"time"
)

const (
	ServiceName     = "reward"
	ServiceAddr     = ":10290"
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
		case convert.LPGPLWPBATTLERESULT:
			handleBattleResult(buf, rank)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}

func handleBattleResult(buf []byte, rank *helpers.RankClient) {
	log.Printf("BATTLERESULT received")
	// Parse
	bufReader := bytes.NewReader(buf)
	recvPacket, _ := convert.NewLwpBattleResult()
	err := binary.Read(bufReader, binary.LittleEndian, recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	id1 := convert.IdCuintToByteArray(recvPacket.Id1)
	id2 := convert.IdCuintToByteArray(recvPacket.Id2)
	var nickname1 string
	var nickname2 string
	convert.CCharArrayToGoString(&recvPacket.Nickname1, &nickname1)
	convert.CCharArrayToGoString(&recvPacket.Nickname2, &nickname2)
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
		log.Printf("rank.Get returned nil!")
		newScore1 = 1
	}
	if oldScore2 != nil {
		newScore2 = oldScore2.Score + 1
		if winner == 2 {
			newScore2++
		}
	} else {
		log.Printf("rank.Get returned nil!")
		newScore2 = 1
	}
	rank.Set(backoff, id1, newScore1, nickname1)
	rank.Set(backoff, id2, newScore2, nickname2)
}
