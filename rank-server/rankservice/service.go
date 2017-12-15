package rankservice

import (
	"github.com/gasbank/laidoff/match-server/rpchelper"
	"github.com/gasbank/laidoff/shared-server"
	"github.com/gasbank/laidoff/db-server/user"
)

type Context struct {
	rpcContext *rpchelper.Context
}

// Set a new score entry
func (c *Context) Set(args *shared_server.ScoreItem, reply *int) error {
	return c.rpcContext.Call("Set", args, reply)
}

// Get score and rank
func (c *Context) Get(args *user.Id, reply *shared_server.ScoreRankItem) error {
	return c.rpcContext.Call("Get", args, reply)
}

// List a leaderboard
func (c *Context) GetLeaderboard(args *shared_server.LeaderboardRequest, reply *shared_server.LeaderboardReply) error {
	return c.rpcContext.Call("GetLeaderboard", args, reply)
}

func New(address string) shared_server.RankService {
	c := new(Context)
	c.rpcContext = rpchelper.New("RankService", address)
	return c
}
