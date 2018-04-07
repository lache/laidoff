#!/bin/bash

./db-server >> db.log 2>&1 &
DB_PID=$!
sleep 1
./match-server >> match.log 2>&1 &
MATCH_PID=$!
sleep 1
./rank-server >> rank.log 2>&1 &
RANK_PID=$!
sleep 1
./reward-server >> reward.log 2>&1 &
REWARD_PID=$!
sleep 1
./push-server >> push.log 2>&1 &
PUSH_PID=$!
sleep 1
./laidoff-server >> laidoff.log 2>&1
LAIDOFF_PID=$!

