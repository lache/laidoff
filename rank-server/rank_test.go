package main

import (
	"testing"
	"runtime/debug"
	"errors"
	"time"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/rank-server/rankservice"
)

func assert(t *testing.T, expected, actual int) {
	if expected != actual {
		debug.PrintStack()
		t.Errorf("assert failed: expected = %v, actual = %v", expected, actual)
	}
}

func assertBool(t *testing.T, expected, actual bool) {
	if expected != actual {
		debug.PrintStack()
		t.Errorf("assert failed: expected = %v, actual = %v", expected, actual)
	}
}

func assertErr(t *testing.T, expected, actual error) {
	if (expected != nil && actual == nil) ||
		(expected == nil && actual != nil) ||
		((expected != nil && actual != nil) && (expected.Error() != actual.Error())) {
		debug.PrintStack()
		t.Errorf("assert failed: expected = %v, actual = %v", expected, actual)
	}
}

type RankTieCount struct {
	rankZeroBased int
	tieCount      int
}

func TestGetRankZeroBasedDesc(t *testing.T) {
	descArr := &[]int{11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	assert(t, 0, getRankZeroBasedDesc(descArr, 999))
	assert(t, 0, getRankZeroBasedDesc(descArr, 12))
	assert(t, 0, getRankZeroBasedDesc(descArr, 11))
	assert(t, 1, getRankZeroBasedDesc(descArr, 10))
	assert(t, 1, getRankZeroBasedDesc(descArr, 9))
	assert(t, 2, getRankZeroBasedDesc(descArr, 8))
	assert(t, 2, getRankZeroBasedDesc(descArr, 7))
	assert(t, 4, getRankZeroBasedDesc(descArr, 6))
	assert(t, 5, getRankZeroBasedDesc(descArr, 5))
	assert(t, 6, getRankZeroBasedDesc(descArr, 4))
	assert(t, 9, getRankZeroBasedDesc(descArr, 3))
	assert(t, 11, getRankZeroBasedDesc(descArr, 2))
	assert(t, 12, getRankZeroBasedDesc(descArr, 1))
	assert(t, 17, getRankZeroBasedDesc(descArr, 0))
	assert(t, 20, getRankZeroBasedDesc(descArr, -1))
	assert(t, 21, getRankZeroBasedDesc(descArr, -2))
	assert(t, 21, getRankZeroBasedDesc(descArr, -999))
}

func assertRankTieCount(t *testing.T, expected RankTieCount, descArr *[]int, score int) {
	actualRank, actualTieCount := getRankAndTieCountZeroBasedDesc(descArr, score)
	assert(t, expected.rankZeroBased, actualRank)
	assert(t, expected.tieCount, actualTieCount)
}

func TestGetRankAndTieCountZeroBasedDesc(t *testing.T) {
	descArr := &[]int{11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	assertRankTieCount(t, RankTieCount{0, 0}, descArr, 999)
	assertRankTieCount(t, RankTieCount{0, 0}, descArr, 12)
	assertRankTieCount(t, RankTieCount{0, 1}, descArr, 11)
	assertRankTieCount(t, RankTieCount{1, 0}, descArr, 10)
	assertRankTieCount(t, RankTieCount{1, 1}, descArr, 9)
	assertRankTieCount(t, RankTieCount{2, 0}, descArr, 8)
	assertRankTieCount(t, RankTieCount{2, 2}, descArr, 7)
	assertRankTieCount(t, RankTieCount{4, 1}, descArr, 6)
	assertRankTieCount(t, RankTieCount{5, 1}, descArr, 5)
	assertRankTieCount(t, RankTieCount{6, 3}, descArr, 4)
	assertRankTieCount(t, RankTieCount{9, 2}, descArr, 3)
	assertRankTieCount(t, RankTieCount{11, 1}, descArr, 2)
	assertRankTieCount(t, RankTieCount{12, 5}, descArr, 1)
	assertRankTieCount(t, RankTieCount{17, 3}, descArr, 0)
	assertRankTieCount(t, RankTieCount{20, 1}, descArr, -1)
	assertRankTieCount(t, RankTieCount{21, 0}, descArr, -2)
	assertRankTieCount(t, RankTieCount{21, 0}, descArr, -999)
}

func assertUpdateScore(t *testing.T, expected RankTieCount, descArr *[]int, oldScore, newScore int) {
	oldLen := len(*descArr)
	actualRank, actualTieCount := updateScoreDesc(descArr, oldScore, newScore)
	assert(t, expected.rankZeroBased, actualRank)
	assert(t, expected.tieCount, actualTieCount)
	assert(t, oldLen, len(*descArr))
}

func TestUpdateScoreDesc(t *testing.T) {
	descArr := &[]int{11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	// {11, 9, 7, 7, 6=>5, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	assertUpdateScore(t, RankTieCount{4, 2}, descArr, 6, 5)
	// {11=>12, 9, 7, 7, 5, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	assertUpdateScore(t, RankTieCount{0, 1}, descArr, 11, 12)
	// {12, 9, 7, 7, 5, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1=>100}
	assertUpdateScore(t, RankTieCount{0, 1}, descArr, -1, 100)
	// {100, 12, 9, 7, 7, 5, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0=>1, 0, 0}
	assertUpdateScore(t, RankTieCount{13, 6}, descArr, 0, 1)
	// {100, 12, 9, 7, 7, 5, 5, 4=>200, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 1, 0, 0}
	assertUpdateScore(t, RankTieCount{0, 1}, descArr, 4, 200)
	// {200=>100, 100, 12, 9, 7, 7, 5, 5, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 1, 0, 0}
	assertUpdateScore(t, RankTieCount{0, 2}, descArr, 200, 100)
	// {100, 100, 12, 9, 7, 7, 5, 5, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 1, 0, 0}
}

func assertInsertNewScore(t *testing.T, expected RankTieCount, descArr *[]int, newScore int) {
	oldLen := len(*descArr)
	actualRank, actualTieCount := insertNewScoreDesc(descArr, newScore)
	assert(t, expected.rankZeroBased, actualRank)
	assert(t, expected.tieCount, actualTieCount)
	assert(t, oldLen+1, len(*descArr))
}

func TestInsertNewScore(t *testing.T) {
	descArr := &[]int{11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	// {11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1}
	assertInsertNewScore(t, RankTieCount{17, 4}, descArr, 0)
	// {11, 9, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, -1}
	assertInsertNewScore(t, RankTieCount{2, 1}, descArr, 8)
	// {11, 9, 8, 7, 7, 6, 5, 4, 4, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, -1}
}

func assertRankDataSet(t *testing.T, expected RankTieCount, rank *RankData, userId byte, score int) {
	actualRank, actualTieCount := rank.Set(user.Id{userId}, score)
	if t != nil {
		assert(t, expected.rankZeroBased, actualRank)
		assert(t, expected.tieCount, actualTieCount)
	}
}

func createRankTestSet(t *testing.T) *RankData {
	rank := newRank()
	assertRankDataSet(t, RankTieCount{0, 1}, rank, 1, 100)
	assertRankDataSet(t, RankTieCount{0, 1}, rank, 1, 200)
	assertRankDataSet(t, RankTieCount{1, 1}, rank, 2, 100)
	assertRankDataSet(t, RankTieCount{2, 1}, rank, 3, 50)
	assertRankDataSet(t, RankTieCount{2, 2}, rank, 4, 50)
	assertRankDataSet(t, RankTieCount{2, 3}, rank, 5, 50)
	assertRankDataSet(t, RankTieCount{5, 1}, rank, 6, 10)
	assertRankDataSet(t, RankTieCount{0, 1}, rank, 7, 250)
	assertRankDataSet(t, RankTieCount{1, 1}, rank, 8, 225)
	assertRankDataSet(t, RankTieCount{4, 4}, rank, 9, 50)
	assertRankDataSet(t, RankTieCount{3, 1}, rank, 6, 105)
	assertRankDataSet(t, RankTieCount{8, 1}, rank, 4, 30)
	assertRankDataSet(t, RankTieCount{0, 2}, rank, 10, 250)
	assertRankDataSet(t, RankTieCount{4, 2}, rank, 7, 100)
	return rank
}

func TestRankData_Set(t *testing.T) {
	createRankTestSet(t)
}

func ExampleRankData_Set() {
	rank := createRankTestSet(nil)
	rank.PrintAll()
	// Output:
	// RankData.0: [10 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 250
	// RankData.1: [8 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 225
	// RankData.2: [1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 200
	// RankData.3: [6 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 105
	// RankData.4: [7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 100
	// RankData.4: [2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 100
	// RankData.6: [9 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 50
	// RankData.6: [5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 50
	// RankData.6: [3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 50
	// RankData.9: [4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 30
}

func ExampleRankData_Remove() {
	rank := createRankTestSet(nil)
	rank.Remove(user.Id{10})
	rank.Remove(user.Id{4})
	rank.Remove(user.Id{7})
	rank.Set(user.Id{7}, 45)
	rank.Set(user.Id{11}, 99)
	rank.PrintAll()
	// Output:
	// RankData.0: [8 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 225
	// RankData.1: [1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 200
	// RankData.2: [6 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 105
	// RankData.3: [2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 100
	// RankData.4: [11 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 99
	// RankData.5: [9 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 50
	// RankData.5: [5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 50
	// RankData.5: [3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 50
	// RankData.8: [7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] 45
}

func TestRankData_Nearest(t *testing.T) {
	rank := createRankTestSet(nil)
	assertNearest(t, 250, 8, 225, nil, rank, 10)
	assertNearest(t, 225, 10, 250, nil, rank, 8)
	assertNearest(t, 200, 8, 225, nil, rank, 1)
	assertNearest(t, 105, 7, 100, nil, rank, 6)
	assertNearest(t, 100, 2, 100, nil, rank, 7)
	assertNearest(t, 100, 7, 100, nil, rank, 2)
	assertNearest(t, 50, 5, 50, nil, rank, 9)
	assertNearest(t, 50, 9, 50, nil, rank, 5)
	assertNearest(t, 50, 5, 50, nil, rank, 3)
	assertNearest(t, 30, 3, 50, nil, rank, 4)
	assertNearest(t, -1, 0, -1, errors.New("id not exist"), rank, 128)
}

func TestRankData_Nearest2(t *testing.T) {
	rank := newRank()
	assertNearest(t, -1, 0, -1, errors.New("rank empty"), rank, 10)
}

func TestRankData_Nearest3(t *testing.T) {
	rank := newRank()
	assertRankDataSet(t, RankTieCount{0, 1}, rank, 1, 100)
	assertNearest(t, -1, 0, -1, errors.New("rank single entry"), rank, 10)
}

func assertNearest(t *testing.T, expectedScore int, expectedId byte, expectedNearestScore int, expectedErr error, rank *RankData, id byte) {
	actualNearestResult, actualErr := rank.Nearest(user.Id{id})
	if actualErr == nil {
		assert(t, expectedScore, actualNearestResult.Score)
		assertUserId(t, user.Id{expectedId}, actualNearestResult.NearestId)
		assert(t, expectedNearestScore, actualNearestResult.NearestScore)
	}
	assertErr(t, expectedErr, actualErr)
}

func assertRankDataGet(t *testing.T, expectedScore, expectedRank, expectedTieCount int, expectedErr error, rank *RankData, userId user.Id) {
	actualScore, actualRank, actualTieCount, actualErr := rank.Get(userId)
	assert(t, expectedScore, actualScore)
	assert(t, expectedRank, actualRank)
	assert(t, expectedTieCount, actualTieCount)
	assertErr(t, expectedErr, actualErr)
}

func TestRankData_Get(t *testing.T) {
	rank := createRankTestSet(t)
	assertRankDataGet(t, 200, 2, 1, nil, rank, user.Id{1})
	assertRankDataGet(t, 100, 4, 2, nil, rank, user.Id{2})
	assertRankDataGet(t, 50, 6, 3, nil, rank, user.Id{3})
	assertRankDataGet(t, 30, 9, 1, nil, rank, user.Id{4})
	assertRankDataGet(t, 50, 6, 3, nil, rank, user.Id{5})
	assertRankDataGet(t, 105, 3, 1, nil, rank, user.Id{6})
	assertRankDataGet(t, 100, 4, 2, nil, rank, user.Id{7})
	assertRankDataGet(t, 225, 1, 1, nil, rank, user.Id{8})
	assertRankDataGet(t, 50, 6, 3, nil, rank, user.Id{9})
	assertRankDataGet(t, 250, 0, 1, nil, rank, user.Id{10})
}

func TestMoveUserIdWithinSlice(t *testing.T) {
	userIdList := &[]user.Id{{1}, {2}, {3}, {4}, {5},}
	assertUserIdArray(t, &[]user.Id{{2}, {1}, {3}, {4}, {5},}, userIdList, 0, 1)
	assertUserIdArray(t, &[]user.Id{{1}, {3}, {4}, {5}, {2},}, userIdList, 0, 4)
	assertUserIdArray(t, &[]user.Id{{2}, {1}, {3}, {4}, {5},}, userIdList, 4, 0)
	assertUserIdArray(t, &[]user.Id{{2}, {1}, {4}, {3}, {5},}, userIdList, 2, 3)
	assertUserIdArray(t, &[]user.Id{{2}, {1}, {3}, {5}, {4},}, userIdList, 2, 4)
	assertUserIdArray(t, &[]user.Id{{2}, {5}, {1}, {3}, {4},}, userIdList, 3, 1)
}

func assertUserIdArray(t *testing.T, expected *[]user.Id, userIdList *[]user.Id, i int, j int) {
	moveUserIdWithinSlice(userIdList, i, j)
	assert(t, len(*expected), len(*userIdList))
	for i, userId := range *userIdList {
		assertUserId(t, (*expected)[i], userId)
	}
}

func assertUserId(t *testing.T, expected user.Id, actual user.Id) {
	if expected != actual {
		debug.PrintStack()
		t.Errorf("assert failed: expected = %v, actual = %v", expected, actual)
	}
}

func TestRankData_RemoveNearestOverlap(t *testing.T) {
	rank := newRank()
	user1 := user.Id{1}
	user2 := user.Id{2}
	rank.Set(user1, 0)
	rank.Set(user2, 100)
	distanceByElapsed := &rankservice.DistanceByElapsed{
		Elapsed: []time.Duration{
			30 * time.Second,
			20 * time.Second,
			10 * time.Second,
			0 * time.Second},
		Distance: []int{
			100,
			50,
			25,
			5},
	}
	now := time.Now()
	result, err := rank.RemoveNearestOverlap(user1, distanceByElapsed, now)
	assertErr(t, nil, err)
	assertBool(t, false, result.Matched)
	assert(t, 2, len(rank.IdArray))
	result, err = rank.RemoveNearestOverlap(user1, distanceByElapsed, now.Add(50*time.Second))
	assertErr(t, nil, err)
	assertBool(t, true, result.Matched)
	assertUserId(t, user1, result.NearestResult.Id)
	assert(t, 0, result.NearestResult.Score)
	assert(t, 1, result.NearestResult.IdArrayIndex)
	assertUserId(t, user2, result.NearestResult.NearestId)
	assert(t, 100, result.NearestResult.NearestScore)
	assert(t, 0, result.NearestResult.NearestIdArrayIndex)
	assert(t, 0, len(rank.IdArray))
}
