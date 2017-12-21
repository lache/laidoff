package rankservice

import (
	"github.com/gasbank/laidoff/match-server/rpchelper"
	"github.com/gasbank/laidoff/shared-server"
	"github.com/gasbank/laidoff/db-server/user"
	"time"
	"log"
)

type Context struct {
	rpcContext *rpchelper.Context
}

type QueueScoreMatchRequest struct {
	Flush             bool              // flush all score match queue
	SetBias           bool              // set match wait time bias (for debugging)
	MatchPoolTimeBias time.Duration     // amount of match wait time bias (for debugging)
	Id                user.Id           // queue user ID
	Score             int               // queue score
	DistanceByElapsed DistanceByElapsed // allowed match score range data
	Update            bool              // true if this is a update request
}

type QueueScoreMatchReply struct {
	RemoveNearestOverlapResult *RemoveNearestOverlapResult
	Err                        error
}

type DistanceByElapsed struct {
	Elapsed  []time.Duration
	Distance []int
}

func (t *DistanceByElapsed) FindDistance(elapsed time.Duration) int {
	for i, e := range t.Elapsed {
		if e <= elapsed {
			return t.Distance[i]
		}
	}
	log.Printf("FindDistance data error")
	return 100
}

type NearestResult struct {
	Id                  user.Id
	Score               int
	IdArrayIndex        int
	NearestId           user.Id
	NearestScore        int
	NearestIdArrayIndex int
}

type RemoveNearestOverlapResult struct {
	Matched        bool
	AlreadyRemoved bool
	NearestResult  *NearestResult
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

func (c *Context) QueueScoreMatch(args *QueueScoreMatchRequest, reply *QueueScoreMatchReply) error {
	return c.rpcContext.Call("QueueScoreMatch", args, reply)
}

func New(address string) RankService {
	c := new(Context)
	c.rpcContext = rpchelper.New("RankService", address)
	return c
}

type RankService interface {
	// Set a new score entry
	Set(args *shared_server.ScoreItem, reply *int) error
	// Get score and rank
	Get(args *user.Id, reply *shared_server.ScoreRankItem) error
	// List a leaderboard
	GetLeaderboard(args *shared_server.LeaderboardRequest, reply *shared_server.LeaderboardReply) error
	// Queue a user for score-based match
	QueueScoreMatch(args *QueueScoreMatchRequest, reply *QueueScoreMatchReply) error
}
