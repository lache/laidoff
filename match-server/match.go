package main

import (
	"log"
	"net"
	"encoding/binary"
	"os"
	"encoding/json"
	"time"
	"math/rand"
	"github.com/gasbank/laidoff/match-server/nickdb"
	"github.com/gasbank/laidoff/match-server/convert"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/match-server/service"
	"github.com/gasbank/laidoff/match-server/handler"
	"github.com/gasbank/laidoff/match-server/config"
	"github.com/gasbank/laidoff/match-server/battle"
	"github.com/gasbank/laidoff/shared-server"
)

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Println("Greetings from match server")
	// Test Db service
	if len(os.Args) > 1 && os.Args[1] == "test" {
		service.CreateNewUserDb()
	}
	// Seed a new random number
	rand.Seed(time.Now().Unix())
	// Load nick name database
	nickDb := nickdb.LoadNickDb()
	// Service List
	serviceList := service.NewServiceList()
	log.Printf("Sample nick: %v", nickdb.PickRandomNick(&nickDb))
	// Load conf.json
	confFile, err := os.Open("conf.json")
	if err != nil {
		log.Fatalf("conf.json open error:%v", err.Error())
	}
	confFileDecoder := json.NewDecoder(confFile)
	conf := config.ServerConfig{}
	err = confFileDecoder.Decode(&conf)
	if err != nil {
		log.Fatalf("conf.json parse error:%v", err.Error())
	}
	// Test RPC
	//testRpc(serviceList)
	// Create 1 vs. 1 match waiting queue
	matchQueue := make(chan user.Agent)
	// Create battle ok queue
	battleOkQueue := make(chan battle.Ok)
	// Ongoing battle map
	ongoingBattleMap := make(map[user.Id]battle.Ok)
	battleService := battle.Service{ Conf: conf }
	// Start match worker goroutine
	go matchWorker(battleService, matchQueue, battleOkQueue, serviceList)
	// Start battle ok worker goroutine
	go battle.OkWorker(conf, battleOkQueue, ongoingBattleMap)
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
			go handleRequest(conf, &nickDb, conn, matchQueue, serviceList, ongoingBattleMap, battleService, battleOkQueue)
		}
	}
}

func testRankRpc(rankService shared_server.RankService, id byte, score int, nickname string) {
	scoreItem := shared_server.ScoreItem{Id: user.Id{id}, Score: score, Nickname: nickname}
	var reply int
	err := rankService.Set(&scoreItem, &reply)
	if err != nil {
		log.Printf("rank rpc error: %v", err.Error())
	} else {
		log.Println(reply)
	}
}

//noinspection GoUnusedFunction
func testRpc(serviceList *service.List) {
	pushToken := shared_server.PushToken{
		Domain:    500,
		PushToken: "test-push-token",
		UserId:    user.Id{1, 2, 3, 4},
	}
	var registerReply int
	log.Println(serviceList.Push.RegisterPushToken(&pushToken, &registerReply))
	testRankRpc(serviceList.Rank, 1, 100, "TestUser1")
	testRankRpc(serviceList.Rank, 2, 200, "TestUser2")
	testRankRpc(serviceList.Rank, 3, 300, "TestUser3")
	testRankRpc(serviceList.Rank, 4, 50, "TestUser4")
	testRankRpc(serviceList.Rank, 5, 20, "TestUser5")
	testRankRpc(serviceList.Rank, 6, 20, "TestUser6")
	testRankRpc(serviceList.Rank, 7, 20, "TestUser7")
	testRankRpc(serviceList.Rank, 8, 10, "TestUser8")
	leaderboardRequest := shared_server.LeaderboardRequest{
		StartIndex: 0,
		Count:      20,
	}
	var leaderboardReply shared_server.LeaderboardReply
	err := serviceList.Rank.GetLeaderboard(&leaderboardRequest, &leaderboardReply)
	if err != nil {
		log.Printf("rank rpc get leaderboard returned error: %v", err.Error())
	} else {
		log.Println(leaderboardReply)
	}
	userId := user.Id{5}
	var scoreRankItem shared_server.ScoreRankItem
	err = serviceList.Rank.Get(&userId, &scoreRankItem)
	if err != nil {
		log.Printf("rank rpc get returned error: %v", err.Error())
	} else {
		log.Println(scoreRankItem)
	}
}

func matchWorker(battleService battle.Service, matchQueue <-chan user.Agent, battleOkQueue chan<- battle.Ok, serviceList *service.List) {
	for {
		log.Printf("Match queue empty")
		c1 := <-matchQueue
		log.Printf("Match queue size 1")
		broadcastPush := shared_server.BroadcastPush{
			Title: "RUMBLE",
			Body:  c1.Db.Nickname + " provokes you!",
		}
		var broadcastReply int
		log.Printf("Broadcast result(return error): %v", serviceList.Push.Broadcast(&broadcastPush, &broadcastReply));
		c2 := <-matchQueue
		log.Printf("Match queue size 2")
		if c1.Conn == c2.Conn {
			if c2.CancelQueue {
				// Send queue cancel success reply
				cancelQueueOkBuf := convert.Packet2Buf(convert.NewLwpCancelQueueOk())
				c2.Conn.Write(cancelQueueOkBuf)
				log.Printf("Nickname '%v' queue cancelled", c2.Db.Nickname)
			} else {
				log.Printf("The same connection sending QUEUE2 twice. Flushing match requests and replying with RETRYQUEUE to the later connection...")
				battle.SendRetryQueue(c2.Conn)
			}
		} else if c1.Db.Id == c2.Db.Id {
			log.Printf("The same user ID sending QUEUE2 twice. Flushing match requests and replying with RETRYQUEUE to the later connection...")
			battle.SendRetryQueue(c2.Conn)
		} else {
			log.Printf("%v and %v matched! (maybe)", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
			maybeMatchedBuf := convert.Packet2Buf(convert.NewLwpMaybeMatched())
			n1, err1 := c1.Conn.Write(maybeMatchedBuf)
			n2, err2 := c2.Conn.Write(maybeMatchedBuf)
			if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
				go battle.CreateBattleInstance(battleService, c1, c2, battleOkQueue)
			} else {
				// Match cannot be proceeded
				checkMatchError(err1, c1.Conn)
				checkMatchError(err2, c2.Conn)
			}
		}
	}
}

func checkMatchError(err error, conn net.Conn) {
	if err != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), err.Error())
	} else {
		battle.SendRetryQueue(conn)
	}
}

func handleRequest(conf config.ServerConfig, nickDb *nickdb.NickDb, conn net.Conn, matchQueue chan<- user.Agent, serviceList *service.List, ongoingBattleMap map[user.Id]battle.Ok, battleService battle.Service, battleOkQueue chan<- battle.Ok) {
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
		case convert.LPGPLWPQUEUE2:
			handler.HandleQueue2(conf, matchQueue, buf, conn, ongoingBattleMap, battleService, battleOkQueue, serviceList.Db)
		case convert.LPGPLWPCANCELQUEUE:
			handler.HandleCancelQueue(matchQueue, buf, conn, ongoingBattleMap, serviceList.Db)
		case convert.LPGPLWPSUDDENDEATH:
			handler.HandleSuddenDeath(conf, buf) // relay 'buf' to battle service
		case convert.LPGPLWPNEWUSER:
			handler.HandleNewUser(nickDb, conn, serviceList.Db)
		case convert.LPGPLWPQUERYNICK:
			handler.HandleQueryNick(buf, conn, serviceList.Rank, serviceList.Db)
		case convert.LPGPLWPPUSHTOKEN:
			handler.HandlePushToken(buf, conn, serviceList)
		case convert.LPGPLWPGETLEADERBOARD:
			handler.HandleGetLeaderboard(buf, conn, serviceList)
		case convert.LPGPLWPSETNICKNAME:
			handler.HandleSetNickname(buf, conn, serviceList.Db)
		}
	}
	conn.Close()
	log.Printf("Conn closed %v", conn.RemoteAddr())
}
