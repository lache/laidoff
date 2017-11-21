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
	"../shared-server"
	"net/rpc"
)

// #include "../src/puckgamepacket.h"
import "C"

const (
	MaxNickLen      = 32
	PushServiceAddr = ":20171"
	RankServiceAddr = ":20172"
)

type UserId [16]byte

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

type ServiceList struct {
	arith *Arith
	rank  *RankClient
}

type UserAgent struct {
	conn   net.Conn
	userDb UserDb
}

type Arith struct {
	client *rpc.Client
}

type RankClient struct {
	client *rpc.Client
}

func (t *Arith) Divide(a, b int) shared_server.Quotient {
	args := &shared_server.Args{a, b}
	var reply shared_server.Quotient
	err := t.client.Call("Arithmetic.Divide", args, &reply)
	if err != nil {
		log.Fatal("arith error:", err)
	}
	return reply
}

func (t *Arith) Multiply(a, b int) int {
	args := &shared_server.Args{a, b}
	var reply int
	err := t.client.Call("Arithmetic.Multiply", args, &reply)
	if err != nil {
		log.Fatal("arith error:", err)
	}
	return reply
}

func (t *Arith) RegisterPushToken(backoff time.Duration, id UserId, domain int, pushToken string) int {
	args := &shared_server.PushToken{domain, pushToken, id}
	var reply int
	err := t.client.Call("PushService.RegisterPushToken", args, &reply)
	if err != nil {
		log.Printf("arith error: %v", err)
		if backoff > 10*time.Second {
			log.Printf("Error - Register Push Token failed: %v", err)
			return 0
		} else if backoff > 0 {
			time.Sleep(backoff)
		}
		t.client, err = dialNewRpc(PushServiceAddr)
		return t.RegisterPushToken(backoff*2, id, domain, pushToken)
	}
	return reply
}

func (t *RankClient) Set(backoff time.Duration, id [16]byte, score int, nickname string) int {
	args := &shared_server.ScoreItem{Id: id, Score: score, Nickname: nickname}
	var reply int
	err := t.client.Call("RankService.Set", args, &reply)
	if err != nil {
		log.Printf("error: %v", err)
		if backoff > 10*time.Second {
			log.Printf("Error: %v - (retry finally failed)", err)
			return 0
		} else if backoff > 0 {
			time.Sleep(backoff)
		}
		t.client, err = dialNewRpc(RankServiceAddr)
		return t.Set(backoff*2, id, score, nickname)
	}
	return reply
}

func (t *RankClient) Get(backoff time.Duration, id UserId) *shared_server.ScoreRankItem {
	args := id
	var reply shared_server.ScoreRankItem
	err := t.client.Call("RankService.Get", args, &reply)
	if err != nil {
		log.Printf("error: %v", err)
		if backoff > 10*time.Second {
			log.Printf("Error: %v - (retry finally failed)", err)
			return nil
		} else if backoff > 0 {
			time.Sleep(backoff)
		}
		t.client, err = dialNewRpc(RankServiceAddr)
		return t.Get(backoff*2, id)
	}
	return &reply
}

func (t *RankClient) GetLeaderboard(backoff time.Duration, startIndex int, count int) *shared_server.LeaderboardReply {
	args := &shared_server.LeaderboardRequest{startIndex, count}
	var reply shared_server.LeaderboardReply
	err := t.client.Call("RankService.GetLeaderboard", args, &reply)
	if err != nil {
		log.Printf("error: %v", err)
		if backoff > 10*time.Second {
			log.Printf("Error: %v - (retry finally failed)", err)
			return nil
		} else if backoff > 0 {
			time.Sleep(backoff)
		}
		t.client, err = dialNewRpc(RankServiceAddr)
		return t.GetLeaderboard(backoff*2, startIndex, count)
	}
	return &reply
}

func dialNewRpc(address string) (*rpc.Client, error) {
	log.Printf("Dial to RPC server %v...", address)
	conn, err := net.Dial("tcp", address)
	if err != nil {
		log.Printf("Connection error: %v", err)
		return nil, err
	}
	return rpc.NewClient(conn), nil
}

func newServiceList() *ServiceList {
	return &ServiceList{
		DialPushService(),
		DialRankService(),
	}
}
func DialPushService() *Arith {
	client, err := dialNewRpc(PushServiceAddr)
	if err != nil {
		log.Printf("dialNewRpc error: %v", err.Error())
	}
	arith := &Arith{client: client}
	return arith
}

func DialRankService() *RankClient {
	client, err := dialNewRpc(RankServiceAddr)
	if err != nil {
		log.Printf("dialNewRpc error: %v", err.Error())
	}
	return &RankClient{client: client}
}

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Println("Greetings from match server")
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	// Seed a new random number
	mathrand.Seed(time.Now().Unix())
	// Load nick name database
	nickDb := Nickdb.LoadNickDb()
	// Service List
	serviceList := newServiceList()
	log.Printf("Sample nick: %v", Nickdb.PickRandomNick(&nickDb))
	// Load conf.json
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
	// Test RPC
	testRpc(serviceList)
	// Create 1 vs. 1 match queue
	matchQueue := make(chan UserAgent)
	// Start match worker goroutine
	go matchWorker(conf, matchQueue)
	// Open TCP service port and listen for game clients
	l, err := net.Listen(conf.ConnType, conf.ConnHost+":"+conf.ConnPort)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Printf("Listening %v for match service...", conf.ConnHost+":"+conf.ConnPort)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			go handleRequest(conf, &nickDb, conn, matchQueue, serviceList)
		}
	}
}

func testRpc(serviceList *ServiceList) {
	log.Println(serviceList.arith.Multiply(5, 6))
	log.Println(serviceList.arith.Divide(500, 10))
	log.Println(serviceList.arith.RegisterPushToken(300*time.Millisecond, UserId{1, 2, 3, 4}, 500, "test-push-token"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{1}, 100, "TestUser1"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{2}, 200, "TestUser2"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{3}, 300, "TestUser3"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{4}, 50, "TestUser4"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{5}, 20, "TestUser5"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{6}, 20, "TestUser6"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{7}, 20, "TestUser7"))
	log.Println(serviceList.rank.Set(300*time.Millisecond, UserId{8}, 10, "한글닉넴8"))
	log.Println(serviceList.rank.GetLeaderboard(300*time.Millisecond, 0, 20))
	log.Println(serviceList.rank.Get(300*time.Millisecond, UserId{5}))
}

func int2ByteArray(v C.int) [4]byte {
	var byteArray [4]byte
	binary.LittleEndian.PutUint32(byteArray[0:], uint32(v))
	return byteArray
}

func createBattleInstance(conf ServerConfig, c1 UserAgent, c2 UserAgent) {
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

	var c1Nickname [C.LW_NICKNAME_MAX_LEN]C.char
	var c2Nickname [C.LW_NICKNAME_MAX_LEN]C.char
	convertGoStringToCCharArray(&c1.userDb.Nickname, &c1Nickname)
	convertGoStringToCCharArray(&c2.userDb.Nickname, &c2Nickname)

	createBattleBuf := packet2Buf(&C.LWPCREATEBATTLE{
		C.ushort(unsafe.Sizeof(C.LWPCREATEBATTLE{})),
		C.LPGP_LWPCREATEBATTLE,
		UserIdToCuint(c1.userDb.Id),
		UserIdToCuint(c2.userDb.Id),
		c1Nickname,
		c2Nickname,
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
		log.Printf("MATCH %v and %v matched successfully!", c1.conn.RemoteAddr(), c2.conn.RemoteAddr())
		c1.conn.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C1_token, 1, c2.userDb.Nickname))
		c2.conn.Write(createMatched2Buf(conf, createBattleOk, createBattleOk.C2_token, 2, c1.userDb.Nickname))
	} else {
		log.Printf("Recv LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE reply corrupted")
	}
}
func createMatched2Buf(conf ServerConfig, createBattleOk C.LWPCREATEBATTLEOK, token C.uint, playerNo C.int, targetNickname string) []byte {
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
		playerNo,
		NicknameToCArray(targetNickname),
	})
}
func packet2Buf(packet interface{}) []byte {
	buf := &bytes.Buffer{}
	binary.Write(buf, binary.LittleEndian, packet)
	return buf.Bytes()
}

func matchWorker(conf ServerConfig, matchQueue <-chan UserAgent) {
	for {
		c1 := <-matchQueue
		c2 := <-matchQueue
		if c1.conn == c2.conn {
			sendRetryQueue(c1.conn)
		} else {
			log.Printf("%v and %v matched! (maybe)", c1.conn.RemoteAddr(), c2.conn.RemoteAddr())
			maybeMatchedBuf := packet2Buf(&C.LWPMAYBEMATCHED{
				C.ushort(unsafe.Sizeof(C.LWPMAYBEMATCHED{})),
				C.LPGP_LWPMAYBEMATCHED,
			})
			n1, err1 := c1.conn.Write(maybeMatchedBuf)
			n2, err2 := c2.conn.Write(maybeMatchedBuf)
			if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
				go createBattleInstance(conf, c1, c2)
			} else {
				// Match cannot be proceeded
				checkMatchError(err1, c1.conn)
				checkMatchError(err2, c2.conn)
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
	Id       UserId
	Created  time.Time
	Nickname string
}

func handleRequest(conf ServerConfig, nickDb *Nickdb.NickDb, conn net.Conn, matchQueue chan<- UserAgent, serviceList *ServiceList) {
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
			handleQueue2(matchQueue, buf, conn)
		case C.LPGP_LWPSUDDENDEATH:
			handleSuddenDeath(conf, buf) // relay 'buf' to battle service
		case C.LPGP_LWPNEWUSER:
			handleNewUser(nickDb, conn)
		case C.LPGP_LWPQUERYNICK:
			handleQueryNick(buf, conn, nickDb)
		case C.LPGP_LWPPUSHTOKEN:
			handlePushToken(buf, conn, serviceList)
		case C.LPGP_LWPGETLEADERBOARD:
			handleGetLeaderboard(buf, conn, serviceList)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}

func convertGoStringToCCharArray(strIn *string, strOut *[C.LW_NICKNAME_MAX_LEN]C.char) {
	for i, b := range []byte(*strIn) {
		(*strOut)[i] = C.char(b)
	}
}

func handleGetLeaderboard(buf []byte, conn net.Conn, serviceList *ServiceList) {
	log.Printf("GETLEADERBOARD received")
	// Parse
	bufReader := bytes.NewReader(buf)
	recvPacket := C.LWPGETLEADERBOARD{}
	err := binary.Read(bufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	startIndex := int(recvPacket.Start_index)
	count := int(recvPacket.Count)
	leaderboardReply := serviceList.rank.GetLeaderboard(300*time.Millisecond, startIndex, count)
	var nicknameList [C.LW_LEADERBOARD_ITEMS_IN_PAGE][C.LW_NICKNAME_MAX_LEN]C.char
	var scoreList [C.LW_LEADERBOARD_ITEMS_IN_PAGE]C.int
	for i, item := range leaderboardReply.Items {
		convertGoStringToCCharArray(&item.Nickname, &nicknameList[i])
		scoreList[i] = C.int(item.Score)
	}
	reply := &C.LWPLEADERBOARD{
		C.ushort(unsafe.Sizeof(C.LWPLEADERBOARD{})),
		C.LPGP_LWPLEADERBOARD,
		C.int(len(leaderboardReply.Items)),
		C.int(leaderboardReply.FirstItemRank),
		C.int(leaderboardReply.FirstItemTieCount),
		nicknameList,
		scoreList,
	}
	replyBuf := packet2Buf(reply)
	conn.Write(replyBuf)
}

func byteArrayToCcharArray256(b []byte) [256]C.char {
	var ret [256]C.char
	bLen := len(b)
	if bLen > 255 {
		bLen = 255
	}
	for i := 0; i < bLen; i++ {
		ret[i] = C.char(b[i])
	}
	return ret
}

func handlePushToken(buf []byte, conn net.Conn, serviceList *ServiceList) {
	log.Printf("PUSHTOKEN received")
	// Parse
	bufReader := bytes.NewReader(buf)
	recvPacket := C.LWPPUSHTOKEN{}
	err := binary.Read(bufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	idBytes := IdCuintToByteArray(recvPacket.Id)
	pushTokenBytes := C.GoBytes(unsafe.Pointer(&recvPacket.Push_token), C.LW_PUSH_TOKEN_LENGTH)
	pushTokenLength := bytes.IndexByte(pushTokenBytes, 0)
	pushToken := string(pushTokenBytes[:pushTokenLength])
	log.Printf("Push token domain %v, token: %v, id: %v", recvPacket.Domain, pushToken, idBytes)
	pushResult := serviceList.arith.RegisterPushToken(300*time.Millisecond, idBytes, int(recvPacket.Domain), pushToken)
	log.Printf("Push result: %v", pushResult)
	if pushResult == 1 {
		sysMsg := []byte(fmt.Sprintf("토큰 등록 완료! %v", pushToken))
		queueOkBuf := packet2Buf(&C.LWPSYSMSG{
			C.ushort(unsafe.Sizeof(C.LWPSYSMSG{})),
			C.LPGP_LWPSYSMSG,
			byteArrayToCcharArray256(sysMsg),
		})
		conn.Write(queueOkBuf)
	}
}

func handleQueue2(matchQueue chan<- UserAgent, buf []byte, conn net.Conn) {
	log.Printf("QUEUE2 received")

	// Parse
	queue2BufReader := bytes.NewReader(buf)
	recvPacket := C.LWPQUEUE2{}
	err := binary.Read(queue2BufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	userDb, err := loadUserDb(recvPacket.Id)
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		// Queue connection
		matchQueue <- UserAgent{conn, *userDb}

		queueOkBuf := packet2Buf(&C.LWPQUEUEOK{
			C.ushort(unsafe.Sizeof(C.LWPQUEUEOK{})),
			C.LPGP_LWPQUEUEOK,
		})
		conn.Write(queueOkBuf)
		log.Printf("Nickname '%v' queued", userDb.Nickname)
	}
}
func handleSuddenDeath(conf ServerConfig, buf []byte) {
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
}

func createNewUser(uuid UserId, nickname string) (*UserDb, *os.File, error) {
	userDb := &UserDb{
		uuid,
		time.Now(),
		nickname,
	}
	uuidStr := IdByteArrayToString(uuid)
	userDbFile, err := os.Create("db/" + uuidStr)
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return nil, nil, err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	return userDb, userDbFile, nil
}

func handleNewUser(nickDb *Nickdb.NickDb, conn net.Conn) {
	log.Printf("NEWUSER received")
	uuid, uuidStr, err := newUuid()
	if err != nil {
		log.Fatalf("new uuid failed: %v", err.Error())
	}
	log.Printf("  - New user guid: %v", uuidStr)
	newNick := Nickdb.PickRandomNick(nickDb)
	cNewNickBytes := NicknameToCArray(newNick)
	newUserDataBuf := packet2Buf(&C.LWPNEWUSERDATA{
		C.ushort(unsafe.Sizeof(C.LWPNEWUSERDATA{})),
		C.LPGP_LWPNEWUSERDATA,
		[4]C.uint{C.uint(binary.BigEndian.Uint32(uuid[0:4])), C.uint(binary.BigEndian.Uint32(uuid[4:8])), C.uint(binary.BigEndian.Uint32(uuid[8:12])), C.uint(binary.BigEndian.Uint32(uuid[12:16]))},
		cNewNickBytes,
	})
	// Write to disk
	var id UserId
	copy(id[:], uuid)
	_, _, err = createNewUser(id, newNick)
	if err != nil {
		log.Fatalf("createNewUser failed: %v", err.Error())
	}
	_, err = conn.Write(newUserDataBuf)
	if err != nil {
		log.Fatalf("NEWUSERDATA send failed: %v", err.Error())
	}
}

func UserIdToCuint(id UserId) [4]C.uint {
	var b [4]C.uint
	b[0] = C.uint(binary.BigEndian.Uint32(id[0:4]))
	b[1] = C.uint(binary.BigEndian.Uint32(id[4:8]))
	b[2] = C.uint(binary.BigEndian.Uint32(id[8:12]))
	b[3] = C.uint(binary.BigEndian.Uint32(id[12:16]))
	return b
}

func IdCuintToByteArray(id [4]C.uint) UserId {
	var b UserId
	binary.BigEndian.PutUint32(b[0:], uint32(id[0]))
	binary.BigEndian.PutUint32(b[4:], uint32(id[1]))
	binary.BigEndian.PutUint32(b[8:], uint32(id[2]))
	binary.BigEndian.PutUint32(b[12:], uint32(id[3]))
	return b
}

func handleQueryNick(buf []byte, conn net.Conn, ndb *Nickdb.NickDb) {
	log.Printf("QUERYNICK received")
	recvPacketBufReader := bytes.NewReader(buf)
	recvPacket := C.LWPQUERYNICK{}
	err := binary.Read(recvPacketBufReader, binary.LittleEndian, &recvPacket)
	if err != nil {
		log.Printf("binary.Read fail: %v", err.Error())
		return
	}
	userDb, err := loadUserDb(recvPacket.Id)
	if err != nil {
		if os.IsNotExist(err) {
			userDb, _, err = createNewUser(IdCuintToByteArray(recvPacket.Id), Nickdb.PickRandomNick(ndb))
			if err != nil {
				log.Fatalf("load user db failed -> create new user failed")
			}
		} else {
			log.Fatalf("load user db failed: %v", err)
		}
	}

	log.Printf("User nick: %v", userDb.Nickname)
	// Send a reply
	nickname := userDb.Nickname
	cNewNickBytes := NicknameToCArray(nickname)
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
func loadUserDb(id [4]C.uint) (*UserDb, error) {
	uuidStr := IdArrayToString(id)
	userDbFile, err := os.Open("db/" + uuidStr)
	if err != nil {
		log.Printf("disk open failed: %v", err.Error())
		return nil, err
	} else {
		defer userDbFile.Close()
		decoder := gob.NewDecoder(userDbFile)
		userDb := &UserDb{}
		decoder.Decode(userDb)
		return userDb, nil
	}
}
func NicknameToCArray(nickname string) [MaxNickLen]C.char {
	var nicknameCchar [MaxNickLen]C.char
	for i, v := range []byte(nickname) {
		if i >= MaxNickLen {
			break
		}
		nicknameCchar[i] = C.char(v)
	}
	nicknameCchar[MaxNickLen-1] = 0
	return nicknameCchar
}

func IdArrayToString(id [4]C.uint) string {
	return fmt.Sprintf("%08x-%08x-%08x-%08x", id[0], id[1], id[2], id[3])
}

func IdByteArrayToString(id UserId) string {
	return fmt.Sprintf("%08x-%08x-%08x-%08x", id[0:4], id[4:8], id[8:12], id[12:16])
}
