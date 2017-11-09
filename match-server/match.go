package main

import (
	"log"
	"net"
	"encoding/binary"
	"bytes"
	"unsafe"
	"os"
	"encoding/json"
	"./nickdb"
	"time"
	"io"
	"fmt"
	mathrand "math/rand"
	cryptorand "crypto/rand"
	"encoding/gob"
)

// #include "../src/puckgamepacket.h"
import "C"

type ServerConfig struct {
	ConnHost string
	ConnPort string
	ConnType string

	BattleServiceHost     string
	BattleServicePort     string
	BattleServiceConnType string

	BattlePublicServiceHost     string
	BattlePublicServicePort     string
	BattlePublicServiceConnType string
}

func main() {
	log.Println("Greetings from match server")
	os.MkdirAll("db", os.ModePerm)
	mathrand.Seed(time.Now().Unix())
	nickDb := Nickdb.LoadNickDb()
	log.Printf("Sample nick: %v", Nickdb.PickRandomNick(&nickDb))
	confFile, err := os.Open("conf.json")
	if err != nil {
		log.Fatalf("conf.json open error:%v", err.Error())
	}
	confFileDecoder := json.NewDecoder(confFile)
	conf := ServerConfig{}
	err = confFileDecoder.Decode(&conf)
	if err != nil {
		log.Fatalf("conf.json parse error:%v", err.Error())
	}
	matchQueue := make(chan net.Conn)
	l, err := net.Listen(conf.ConnType, conf.ConnHost+":"+conf.ConnPort)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Println("Listening on " + conf.ConnHost + ":" + conf.ConnPort)
	go matchWorker(conf, matchQueue)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conf, &nickDb, conn, matchQueue)
		}
	}
}

func int2ByteArray(v C.int) [4]byte {
	var byteArray [4]byte
	binary.LittleEndian.PutUint32(byteArray[0:], uint32(v))
	return byteArray
}

func createBattleInstance(conf ServerConfig, c1 net.Conn, c2 net.Conn) {
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
	createBattleBuf := packet2Buf(&C.LWPCREATEBATTLE{
		C.ushort(unsafe.Sizeof(C.LWPCREATEBATTLE{})),
		C.LPGP_LWPCREATEBATTLE,
	})
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
		log.Printf("MATCH %v and %v matched successfully!", c1.RemoteAddr(), c2.RemoteAddr())
		c1.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C1_token))
		c2.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C2_token))
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}
func createMatched2Buf(conf ServerConfig, createBattleOk C.LWPCREATEBATTLEOK, token C.uint) []byte {
	publicAddr, err := net.ResolveTCPAddr(conf.BattlePublicServiceConnType, conf.BattlePublicServiceHost+":"+conf.BattlePublicServicePort)
	if err != nil {
		log.Panicf("BattlePublicService conf parse error: %v", err.Error())
	}
	publicAddrIpv4 := publicAddr.IP.To4()
	return packet2Buf(C.LWPMATCHED2{
		C.ushort(unsafe.Sizeof(C.LWPMATCHED2{})),
		C.LPGP_LWPMATCHED2,
		C.ushort(publicAddr.Port), // createBattleOk.Port
		C.ushort(0),               // padding
		[4]C.uchar{C.uchar(publicAddrIpv4[0]), C.uchar(publicAddrIpv4[1]), C.uchar(publicAddrIpv4[2]), C.uchar(publicAddrIpv4[3]),},
		createBattleOk.Battle_id,
		token,
	})
}
func packet2Buf(packet interface{}) []byte {
	buf := &bytes.Buffer{}
	binary.Write(buf, binary.LittleEndian, packet)
	return buf.Bytes()
}

func matchWorker(conf ServerConfig, matchQueue <-chan net.Conn) {
	for {
		c1 := <-matchQueue
		c2 := <-matchQueue
		if c1 == c2 {
			sendRetryQueue(c1)
		} else {
			log.Printf("%v and %v matched! (maybe)", c1.RemoteAddr(), c2.RemoteAddr())
			maybeMatchedBuf := packet2Buf(&C.LWPMAYBEMATCHED{
				C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
				C.LPGP_LWPMAYBEMATCHED,
			})
			n1, err1 := c1.Write(maybeMatchedBuf)
			n2, err2 := c2.Write(maybeMatchedBuf)
			if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
				go createBattleInstance(conf, c1, c2)
			} else {
				// Match cannot be proceeded
				checkMatchError(err1, c1)
				checkMatchError(err2, c2)
			}
		}
	}
}

func checkMatchError(err error, conn net.Conn) {
	if err != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), err.Error())
	} else {
		sendRetryQueue(conn)
	}
}
func sendRetryQueue(conn net.Conn) {
	retryQueueBuf := packet2Buf(&C.LWPRETRYQUEUE{
		C.ushort(unsafe.Sizeof(C.LWPRETRYQUEUE{})),
		C.LPGP_LWPRETRYQUEUE,
	})
	_, retrySendErr := conn.Write(retryQueueBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry match packet to client", conn.RemoteAddr())
	}
}

func newUuid() ([]byte, string, error) {
	uuid := make([]byte, 16)

	n, err := io.ReadFull(cryptorand.Reader, uuid)
	if n != len(uuid) || err != nil {
		return nil, "", err
	}
	// variant bits; see section 4.1.1
	uuid[8] = uuid[8]&^0xc0 | 0x80
	// version 4 (pseudo-random); see section 4.1.3
	uuid[6] = uuid[6]&^0xf0 | 0x40
	return uuid, fmt.Sprintf("%08x-%08x-%08x-%08x", uuid[0:4], uuid[4:8], uuid[8:12], uuid[12:16]), nil
}

type UserDb struct {
	Id       []byte
	Created  time.Time
	Nickname string
}

func handleRequest(conf ServerConfig, nickDb *Nickdb.NickDb, conn net.Conn, matchQueue chan<- net.Conn) {
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
		case C.LPGP_LWPQUEUE2:
			log.Printf("QUEUE2 received")
			matchQueue <- conn
			queueOkBuf := packet2Buf(&C.LWPQUEUEOK{
				C.ushort(unsafe.Sizeof(C.LWPQUEUEOK{})),
				C.LPGP_LWPQUEUEOK,
			})
			conn.Write(queueOkBuf)
		case C.LPGP_LWPSUDDENDEATH:
			log.Printf("SUDDENDEATH received")
			tcpAddr, err := net.ResolveTCPAddr(conf.BattleServiceConnType, conf.BattleServiceHost+":"+conf.BattleServicePort)
			if err != nil {
				log.Fatalf("ResolveTCPAddr error! - %v", err.Error())
			}
			conn, err := net.DialTCP("tcp", nil, tcpAddr)
			if err != nil {
				log.Fatalf("DialTCP error! - %v", err.Error())
			}
			_, err = conn.Write(buf)
			if err != nil {
				log.Fatalf("Send SUDDENDEATH failed")
			}
		case C.LPGP_LWPNEWUSER:
			handleNewUser(nickDb, conn)
		case C.LPGP_LWPQUERYNICK:
			handleQueryNick(buf, conn)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}
func handleNewUser(nickDb *Nickdb.NickDb, conn net.Conn) {
	log.Printf("NEWUSER received")
	uuid, uuidStr, err := newUuid()
	if err != nil {
		log.Fatalf("new uuid failed: %v", err.Error())
	}
	log.Printf("  - New user guid: %v", uuidStr)
	newNick := Nickdb.PickRandomNick(nickDb)
	newNickBytes := []byte(newNick)
	const maxNickLen = 32
	var cNewNickBytes [maxNickLen]C.char
	for i, v := range newNickBytes {
		if i >= maxNickLen {
			break
		}
		cNewNickBytes[i] = C.char(v)
	}
	cNewNickBytes[maxNickLen-1] = 0
	newUserDataBuf := packet2Buf(&C.LWPNEWUSERDATA{
		C.ushort(unsafe.Sizeof(C.LWPNEWUSERDATA{})),
		C.LPGP_LWPNEWUSERDATA,
		[4]C.uint{C.uint(binary.BigEndian.Uint32(uuid[0:4])), C.uint(binary.BigEndian.Uint32(uuid[4:8])), C.uint(binary.BigEndian.Uint32(uuid[8:12])), C.uint(binary.BigEndian.Uint32(uuid[12:16]))},
		cNewNickBytes,
	})
	// Write to disk
	userDb := UserDb{
		uuid,
		time.Now(),
		newNick,
	}
	userDbFile, err := os.Create("db/" + uuidStr)
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	_, err = conn.Write(newUserDataBuf)
	if err != nil {
		log.Fatalf("NEWUSERDATA send failed: %v", err.Error())
	}
}
func handleQueryNick(buf []byte, conn net.Conn) {
	log.Printf("QUERYNICK received")
	recvPacketBufReader := bytes.NewReader(buf)
	recvPacket := C.LWPQUERYNICK{}
	err := binary.Read(recvPacketBufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	uuidStr := fmt.Sprintf("%08x-%08x-%08x-%08x", recvPacket.Id[0], recvPacket.Id[1], recvPacket.Id[2], recvPacket.Id[3])
	userDbFile, err := os.Open("db/" + uuidStr)
	if err != nil {
		log.Printf("disk open failed: %v", err.Error())
	} else {
		defer userDbFile.Close()
		decoder := gob.NewDecoder(userDbFile)
		userDb := &UserDb{}
		decoder.Decode(userDb)
		log.Printf("User nick: %v", userDb.Nickname)
		// Send a reply
		const maxNickLen = 32
		var cNewNickBytes [maxNickLen]C.char
		for i, v := range []byte(userDb.Nickname) {
			if i >= maxNickLen {
				break
			}
			cNewNickBytes[i] = C.char(v)
		}
		cNewNickBytes[maxNickLen-1] = 0
		nickBuf := packet2Buf(&C.LWPNICK{
			C.ushort(unsafe.Sizeof(C.LWPNICK{})),
			C.LPGP_LWPNICK,
			cNewNickBytes,
		})
		_, err = conn.Write(nickBuf)
		if err != nil {
			log.Fatalf("LWPNICK send failed: %v", err.Error())
		}
	}
}
