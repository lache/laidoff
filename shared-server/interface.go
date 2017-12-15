package shared_server

import "github.com/gasbank/laidoff/db-server/user"

type Arith interface {
	Multiply(args *Args, reply *int) error
	Divide(args *Args, quo *Quotient) error
}

type PushService interface {
	RegisterPushToken(args *PushToken, reply *int) error
	Broadcast(args *BroadcastPush, reply *int) error
}

type RankService interface {
	// Set a new score entry
	Set(args *ScoreItem, reply *int) error
	// Get score and rank
	Get(args *user.Id, reply *ScoreRankItem) error
	// List a leaderboard
	GetLeaderboard(args *LeaderboardRequest, reply *LeaderboardReply) error
}
