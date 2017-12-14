package helpers

import (
	"time"
	"log"
	"net/rpc"
	"net"
	"../../shared-server"
	"strings"
)

type RankClient struct {
	client *rpc.Client
	addr string
}

func NewRankClient(address string) *RankClient {
	rankRpc, err := dialNewRpc(address)
	if err != nil {
		log.Fatalf("Rank service connection error: %v", err.Error())
	}
	return &RankClient{rankRpc, address }
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
		t.client, err = dialNewRpc(t.addr)
		return t.Set(backoff*2, id, score, nickname)
	}
	return reply
}

func (t *RankClient) Get(backoff time.Duration, id [16]byte) *shared_server.ScoreRankItem {
	args := id
	var reply shared_server.ScoreRankItem
	err := t.client.Call("RankService.Get", args, &reply)
	if err != nil {
		if strings.Compare(err.Error(), "id not exist") == 0 {
			return nil
		} else {
			log.Printf("error: %v", err)
			if backoff > 10*time.Second {
				log.Printf("Error: %v - (retry finally failed)", err)
				return nil
			} else if backoff > 0 {
				time.Sleep(backoff)
			}
			t.client, err = dialNewRpc(t.addr)
			return t.Get(backoff*2, id)
		}
	}
	return &reply
}
