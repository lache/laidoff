package main

import (
	"sort"
	"log"
	"errors"
	"net/rpc"
	"../shared-server"
	"net"
)

type UserId [16]byte

type Rank struct {
	IdScoreMap    map[UserId]int
	ScoreArray    []int
	IdArray       []UserId
	NicknameArray []string
}

func (t *Rank) Set(id UserId, newScore int) (int, int) {
	return t.SetWithNickname(id, newScore, "")
}

func (t *Rank) SetWithNickname(id UserId, newScore int, nickname string) (int, int) {
	if oldScore, ok := t.IdScoreMap[id]; ok {
		// Existing id
		oldRank, oldTieCount := getRankAndTieCountZeroBasedDesc(&t.ScoreArray, oldScore)
		if oldScore == newScore {
			// The same score; nothing to do
			return oldRank, oldTieCount
		}
		// TODO Should change to binary search from linear search
		oldIdArrayIndex := -1
		for i := oldRank; i < oldRank+oldTieCount; i++ {
			if t.IdArray[i] == id {
				oldIdArrayIndex = i
				break
			}
		}
		if oldIdArrayIndex < 0 {
			log.Fatal("CRITICAL ERROR: oldIdArrayIndex not found!")
		} else {
			// Update to new score
			t.IdScoreMap[id] = newScore
			newRank, newTieCount := updateScoreDesc(&t.ScoreArray, oldScore, newScore)
			moveUserIdWithinSlice(&t.IdArray, oldIdArrayIndex, newRank)
			t.NicknameArray[oldIdArrayIndex] = nickname
			moveStringWithinSlice(&t.NicknameArray, oldIdArrayIndex, newRank)
			// Update nickname (if changed)
			//t.NicknameArray[newRank] = nickname
			return newRank, newTieCount
		}
		return -1, -1
	} else {
		// New id, new score
		t.IdScoreMap[id] = newScore
		rank, tieCount := insertNewScoreDesc(&t.ScoreArray, newScore)
		insertUserIdToSlice(&t.IdArray, rank, id)
		insertStringToSlice(&t.NicknameArray, rank, nickname)
		return rank, tieCount
	}
	// Unreachable code
	return -1, -1
}

func (t *Rank) Get(id UserId) (int, int, int, error) {
	if oldScore, ok := t.IdScoreMap[id]; ok {
		rank, tieCount := getRankAndTieCountZeroBasedDesc(&t.ScoreArray, oldScore)
		return oldScore, rank, tieCount, nil
	}
	return -1, -1, -1, errors.New("id not exist")
}

func (t *Rank) PrintAll() {
	rank := 0
	tieCount := 1
	for i, c := 0, len(t.IdArray); i < c; i++ {
		log.Printf("Rank.%v: %v %v", rank, t.IdArray[i], t.ScoreArray[i])
		if i < c-1 {
			if t.ScoreArray[i+1] == t.ScoreArray[i] {
				tieCount++
			} else {
				rank += tieCount
				tieCount = 1
			}
		}
	}
}

func getRankZeroBasedDesc(descArr *[]int, score int) int {
	return sort.Search(len(*descArr), func(i int) bool { return score >= (*descArr)[i] })
}

func getNextRankZeroBasedDesc(descArr *[]int, score int) int {
	return sort.Search(len(*descArr), func(i int) bool { return score > (*descArr)[i] })
}

func getRankAndTieCountZeroBasedDesc(descArr *[]int, score int) (int, int) {
	rankZeroBased := getRankZeroBasedDesc(descArr, score)
	tieCount := getNextRankZeroBasedDesc(descArr, score) - rankZeroBased
	return rankZeroBased, tieCount
}

func insertScoreToSlice(s *[]int, i int, x int) {
	*s = append(*s, 0)
	copy((*s)[i+1:], (*s)[i:])
	(*s)[i] = x
}

func insertUserIdToSlice(s *[]UserId, i int, x UserId) {
	*s = append(*s, UserId{})
	copy((*s)[i+1:], (*s)[i:])
	(*s)[i] = x
}

func insertStringToSlice(s *[]string, i int, x string) {
	*s = append(*s, "")
	copy((*s)[i+1:], (*s)[i:])
	(*s)[i] = x
}

func removeUserIdFromSlice(s *[]UserId, i int) {
	*s = append((*s)[0:i], (*s)[i+1:]...)
}

func removeStringFromSlice(s *[]string, i int) {
	*s = append((*s)[0:i], (*s)[i+1:]...)
}

func moveUserIdWithinSlice(s *[]UserId, i, j int) {
	if i == j {
		return
	}
	m := (*s)[i]
	removeUserIdFromSlice(s, i)
	insertUserIdToSlice(s, j, m)
}

func moveStringWithinSlice(s *[]string, i, j int) {
	if i == j {
		return
	}
	m := (*s)[i]
	removeStringFromSlice(s, i)
	insertStringToSlice(s, j, m)
}

func updateScoreDesc(descArr *[]int, oldScore, newScore int) (int, int) {
	if oldScore == newScore {
		return -1, -1
	}
	oldScoreRank, oldScoreTieCount := getRankAndTieCountZeroBasedDesc(descArr, oldScore)
	if oldScore > newScore {
		descArrPre := (*descArr)[0:(oldScoreRank + oldScoreTieCount - 1)]
		descArrPost := (*descArr)[oldScoreRank+oldScoreTieCount:]
		insertRank, insertTieCount := getRankAndTieCountZeroBasedDesc(&descArrPost, newScore)
		insertScoreToSlice(&descArrPost, insertRank, newScore)
		*descArr = append(descArrPre, descArrPost...)
		return len(descArrPre) + insertRank, insertTieCount + 1
	}
	if oldScore < newScore {
		descArrPre := (*descArr)[0:oldScoreRank]
		descArrPost := (*descArr)[oldScoreRank+1:]
		insertRank, insertTieCount := getRankAndTieCountZeroBasedDesc(&descArrPre, newScore)
		insertScoreToSlice(&descArrPre, insertRank, newScore)
		*descArr = append(descArrPre, descArrPost...)
		return insertRank, insertTieCount + 1
	}
	// Unreachable code
	return -1, -1
}

func insertNewScoreDesc(descArr *[]int, newScore int) (int, int) {
	insertRank, insertTieCount := getRankAndTieCountZeroBasedDesc(descArr, newScore)
	insertScoreToSlice(descArr, insertRank, newScore)
	return insertRank, insertTieCount + 1
}

func newRank() *Rank {
	return &Rank{
		IdScoreMap: make(map[UserId]int),
		ScoreArray: make([]int, 0),
		IdArray:    make([]UserId, 0),
	}
}

type RankService struct {
	rank *Rank
}

func (t *RankService) Set(args *shared_server.ScoreItem, reply *int) error {
	rank, _ := t.rank.SetWithNickname(args.Id, args.Score, args.Nickname)
	*reply = rank
	return nil
}

func (t *RankService) Get(args *[16]byte, reply *shared_server.ScoreRankItem) error {
	score, rank, _, err := t.rank.Get(*args)
	if err != nil {
		log.Printf("Get failed: %v", err)
		reply.Id = *args
	} else {
		reply.Id = *args
		reply.Score = score
		reply.Rank = rank
	}
	return nil
}

func (t *RankService) GetLeaderboard(args *shared_server.LeaderboardRequest, reply *shared_server.LeaderboardReply) error {
	scoreCount := len(t.rank.ScoreArray)
	if scoreCount <= args.StartIndex || args.StartIndex < 0 {
		log.Printf("StartIndex out of bounds error")
		return nil
	}
	if args.Count <= 0 || args.Count > 100 {
		log.Printf("Count out of bounds error")
		return nil
	}

	firstId := t.rank.IdArray[args.StartIndex]
	_, rank, tieCount, err := t.rank.Get(firstId)
	if err != nil {
		log.Printf("Get error")
	} else {
		reply.FirstItemRank = rank
		reply.FirstItemTieCount = tieCount
		items := make([]shared_server.LeaderboardItem, 0)
		c := args.Count
		if args.StartIndex+c > scoreCount {
			c = scoreCount - args.StartIndex
		}
		for i := 0; i < c; i++ {
			items = append(items, shared_server.LeaderboardItem{
				Nickname: t.rank.NicknameArray[args.StartIndex+i],
				Score:    t.rank.ScoreArray[args.StartIndex+i],
			})
		}
		reply.Items = items
	}
	return nil
}

func main() {
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	//selfTest()
	server := rpc.NewServer()
	rankService := &RankService{
		newRank(),
	}
	server.RegisterName("RankService", rankService)
	addr := ":20172"
	log.Printf("Listening %v for rank service...", addr)
	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", addr)
	if e != nil {
		log.Fatal("listen error:", e)
	}
	server.Accept(l)
}
func selfTest() {
	descArr := &[]int{11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	log.Print(descArr)
	log.Printf("arr len: %v", len(*descArr))
	log.Printf("rank zero based: %v", getRankZeroBasedDesc(descArr, 8))
	r, t := getRankAndTieCountZeroBasedDesc(descArr, 8)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = updateScoreDesc(descArr, 6, 5)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = updateScoreDesc(descArr, 11, 12)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = updateScoreDesc(descArr, -1, 100)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = updateScoreDesc(descArr, 0, 1)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = updateScoreDesc(descArr, 4, 200)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = updateScoreDesc(descArr, 200, 100)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = insertNewScoreDesc(descArr, 0)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	r, t = insertNewScoreDesc(descArr, 8)
	log.Print(descArr)
	log.Printf("rank zero based: %v, tie count: %v", r, t)
	rank := newRank()
	// Rank.Set test
	r, t = rank.Set(UserId{1}, 100)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{1}, 200)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{2}, 100)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{3}, 50)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{4}, 50)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{5}, 50)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{6}, 10)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{7}, 250)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{8}, 225)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{9}, 50)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{6}, 105)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{4}, 30)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{10}, 250)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	r, t = rank.Set(UserId{7}, 100)
	log.Printf("RANK: rank zero based: %v, tie count: %v", r, t)
	// Rank.PrintAll test
	rank.PrintAll()
	// Rank.Get test
	score, r, t, err := rank.Get(UserId{1})
	if err != nil {
		log.Printf("rank get error! %v", err.Error())
	} else {
		log.Printf("RANK GET: score %v, rank zero based: %v, tie count: %v", score, r, t)
	}
	userIdList := &[]UserId{
		UserId{1},
		UserId{2},
		UserId{3},
		UserId{4},
		UserId{5},
	}
	log.Print(userIdList)
	moveUserIdWithinSlice(userIdList, 0, 1)
	log.Print(userIdList)
	moveUserIdWithinSlice(userIdList, 0, 4)
	log.Print(userIdList)
	moveUserIdWithinSlice(userIdList, 4, 0)
	log.Print(userIdList)
	moveUserIdWithinSlice(userIdList, 2, 3)
	log.Print(userIdList)
	moveUserIdWithinSlice(userIdList, 2, 4)
	log.Print(userIdList)
	moveUserIdWithinSlice(userIdList, 3, 1)
	log.Print(userIdList)
}
