package service

import (
	"net/rpc"
	"time"
	"log"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/shared-server"
	"net"
)

const (
	PushServiceAddr = ":20171"
	RankServiceAddr = ":20172"
)

type List struct {
	Arith *Arith
	Rank  *RankClient
}

type Arith struct {
	client *rpc.Client
}

type RankClient struct {
	client *rpc.Client
}

func (t *Arith) Divide(a, b int) shared_server.Quotient {
	args := &shared_server.Args{A: a, B: b}
	var reply shared_server.Quotient
	err := t.client.Call("Arithmetic.Divide", args, &reply)
	if err != nil {
		log.Fatal("Arith error:", err)
	}
	return reply
}

func (t *Arith) Multiply(a, b int) int {
	args := &shared_server.Args{A: a, B: b}
	var reply int
	err := t.client.Call("Arithmetic.Multiply", args, &reply)
	if err != nil {
		log.Fatal("Arith error:", err)
	}
	return reply
}

func (t *Arith) RegisterPushToken(backoff time.Duration, id user.Id, domain int, pushToken string) int {
	args := &shared_server.PushToken{Domain: domain, PushToken: pushToken, UserId: id}
	var reply int
	err := t.client.Call("PushService.RegisterPushToken", args, &reply)
	if err != nil {
		log.Printf("Arith error: %v", err)
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

func (t *Arith) Broadcast(backoff time.Duration, title, body string) int {
	args := &shared_server.BroadcastPush{Title: title, Body: body}
	var reply int
	err := t.client.Call("PushService.Broadcast", args, &reply)
	if err != nil {
		log.Printf("Arith error: %v", err)
		if backoff > 10*time.Second {
			log.Printf("Error - Broadcast Push failed: %v", err)
			return 0
		} else if backoff > 0 {
			time.Sleep(backoff)
		}
		t.client, err = dialNewRpc(PushServiceAddr)
		return t.Broadcast(backoff*2, title, body)
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

func (t *RankClient) Get(backoff time.Duration, id user.Id) *shared_server.ScoreRankItem {
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
	args := &shared_server.LeaderboardRequest{StartIndex: startIndex, Count: count}
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

func NewServiceList() *List {
	return &List{
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

func CreateNewUserDb() {
	client, err := dialNewRpc(":20181")
	if err != nil {
		log.Printf("rpc connection error")
		return
	}
	var userId user.Id
	// Create test
	{
		var reply user.Db
		err = client.Call("DbService.Create", 0, &reply)
		if err != nil {
			log.Printf("rpc call error: %v", err.Error())
		} else {
			log.Printf("DbService.Create reply: %v", reply.Nickname)
		}
		userId = reply.Id
	}
	// Create test
	{
		var reply user.Db
		err = client.Call("DbService.Get", &userId, &reply)
		if err != nil {
			log.Printf("rpc call error: %v", err.Error())
		} else {
			log.Printf("DbService.Get reply: %v", reply.Nickname)
		}
	}
	// Lease & Write test
	{
		var reply user.LeaseDb
		// Lease
		err = client.Call("DbService.Lease", &userId, &reply)
		if err != nil {
			log.Printf("rpc call error: %v", err.Error())
		} else {
			log.Printf("DbService.Lease reply: Nickname %v, Lease ID %v",
				reply.Db.Nickname, reply.LeaseId)
		}
		// Write
		var writeReply int
		reply.Db.Nickname = "CanYouSeeMe"
		err = client.Call("DbService.Write", &reply, &writeReply)
		if err != nil {
			log.Printf("rpc call error: %v", err.Error())
		} else {
			log.Printf("DbService.Write reply: %v", writeReply)
		}
	}
}
