package main

import (
	"sort"
	"log"
	"errors"
)

type UserId [1]byte

type Rank struct {
	IdScoreMap      map[UserId]int
	ScoreArray      []int
	IdArray         []UserId
}

func (t *Rank) Set(id UserId, newScore int) (int, int) {
	if oldScore, ok := t.IdScoreMap[id]; ok {
		// Existing id
		if oldScore == newScore {
			// The same score; nothing to do
			return getRankAndTieCountZeroBasedDesc(&t.ScoreArray, oldScore)
		}
		// Update to new score
		t.IdScoreMap[id] = newScore
		rank, tieCount := updateScoreDesc(&t.ScoreArray, oldScore, newScore)
		return rank, tieCount
	} else {
		// New id, new score
		t.IdScoreMap[id] = newScore
		rank, tieCount := insertNewScoreDesc(&t.ScoreArray, newScore)
		insertUserIdToSlice(&t.IdArray, rank, id)
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

func removeUserIdFromSlice(s *[]UserId, i int) {
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
		IdScoreMap:      make(map[UserId]int),
		ScoreArray:      make([]int, 0),
		IdArray:         make([]UserId, 0),
	}
}

func main() {
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
