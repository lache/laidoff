package shared_server

type Args struct {
	A, B int
}

type Quotient struct {
	Quo, Rem int
}

type PushToken struct {
	Domain    int
	PushToken string
	UserId    [16]byte
}

type BroadcastPush struct {
	Title string
	Body  string
}

type ScoreItem struct {
	Id       [16]byte
	Score    int
	Nickname string
}

type ScoreRankItem struct {
	Id    [16]byte
	Score int
	Rank  int
}

type LeaderboardRequest struct {
	StartIndex int
	Count      int
}

type LeaderboardItem struct {
	Nickname string
	Score    int
}

type LeaderboardReply struct {
	FirstItemRank     int
	FirstItemTieCount int
	Items             []LeaderboardItem
}
