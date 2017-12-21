package main

import (
	"testing"
	"github.com/gasbank/laidoff/match-server/service"
	"github.com/gasbank/laidoff/rank-server/rankservice"
	"github.com/gasbank/laidoff/db-server/user"
	"time"
	"github.com/stretchr/testify/assert"
)

func queueScoreMatch(id byte, score int, serviceList *service.List, distanceByElapsed *rankservice.DistanceByElapsed) (rankservice.QueueScoreMatchReply, error) {
	request := rankservice.QueueScoreMatchRequest{
		Id:                user.Id{id},
		Score:             score,
		DistanceByElapsed: *distanceByElapsed,
	}
	var reply rankservice.QueueScoreMatchReply
	err := serviceList.Rank.QueueScoreMatch(&request, &reply)
	return reply, err
}

func flushQueueScoreMatch(serviceList *service.List) (rankservice.QueueScoreMatchReply, error) {
	request := rankservice.QueueScoreMatchRequest{
		Flush: true,
	}
	var reply rankservice.QueueScoreMatchReply
	err := serviceList.Rank.QueueScoreMatch(&request, &reply)
	return reply, err
}

func setMatchPoolTimeBias(serviceList *service.List, bias time.Duration) (rankservice.QueueScoreMatchReply, error) {
	request := rankservice.QueueScoreMatchRequest{
		SetBias:           true,
		MatchPoolTimeBias: bias,
	}
	var reply rankservice.QueueScoreMatchReply
	err := serviceList.Rank.QueueScoreMatch(&request, &reply)
	return reply, err
}

func assertNotMatchedFirstQueue(t *testing.T, reply rankservice.QueueScoreMatchReply, err error) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.Nil(t, reply.RemoveNearestOverlapResult)
}

func assertNotMatched(t *testing.T, reply rankservice.QueueScoreMatchReply, err error, nearestId byte) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.Equal(t, false, reply.RemoveNearestOverlapResult.Matched, "Matched")
	assert.Equal(t, user.Id{nearestId}, reply.RemoveNearestOverlapResult.NearestResult.NearestId)
}

func assertMatched(t *testing.T, reply rankservice.QueueScoreMatchReply, err error, matchedId byte) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.Equal(t, true, reply.RemoveNearestOverlapResult.Matched, "Matched")
	assert.Equal(t, user.Id{matchedId}, reply.RemoveNearestOverlapResult.NearestResult.NearestId)
}

func TestMatchRpc(t *testing.T) {
	serviceList := service.NewServiceList()
	distanceByElapsed := rankservice.DistanceByElapsed{
		Elapsed:  []time.Duration{30 * time.Second, 20 * time.Second, 10 * time.Second, 0 * time.Second},
		Distance: []int{100, 50, 25, 5},
	}
	// Flush queue (for easy debugging)
	_, err := flushQueueScoreMatch(serviceList)
	assert.Equal(t, nil, err)
	// Case 1 - matched immediately after second user queued
	// First user
	reply1, err := queueScoreMatch(1, 100, serviceList, &distanceByElapsed)
	assertNotMatchedFirstQueue(t, reply1, err)
	// Second user
	reply2, err := queueScoreMatch(2, 99, serviceList, &distanceByElapsed)
	assertMatched(t, reply2, err, 1)
	// Case 2 - matched immediately after second user queued with some time interval by first user
	// First user
	reply3, err := queueScoreMatch(3, 100, serviceList, &distanceByElapsed)
	assertNotMatchedFirstQueue(t, reply3, err)
	// Second user
	reply4a, err := queueScoreMatch(4, 80, serviceList, &distanceByElapsed)
	assertNotMatched(t, reply4a, err, 3)
	setMatchPoolTimeBias(serviceList, 15*time.Second)
	reply4b, err := queueScoreMatch(4, 80, serviceList, &distanceByElapsed)
	assertMatched(t, reply4b, err, 3)
}
