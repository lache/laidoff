package shared_server

type Arith interface {
	Multiply(args *Args, reply *int) error
	Divide(args *Args, quo *Quotient) error
}

type PushService interface {
	RegisterPushToken(args *PushToken, reply *int) error
}

type RankService interface {
	// Set a new score entry
	Set(args *ScoreItem, reply *int) error
	// Get score and rank
	Get(args *[16]byte, reply *ScoreRankItem) error
	// List a leaderboard
	GetLeaderboard(args *LeaderboardRequest, reply *LeaderboardReply) error
}
