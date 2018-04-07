#!/bin/bash

cd db
./db-server >> db.log 2>&1 &
DB_PID=$!
cd ..
sleep 1

cd match
./match-server >> match.log 2>&1 &
MATCH_PID=$!
cd ..
sleep 1

cd rank
./rank-server >> rank.log 2>&1 &
RANK_PID=$!
cd ..
sleep 1

cd reward
./reward-server >> reward.log 2>&1 &
REWARD_PID=$!
cd ..
sleep 1

cd push
./push-server >> push.log 2>&1 &
PUSH_PID=$!
cd ..
sleep 1

cd battle
./laidoff-server >> laidoff.log 2>&1
LAIDOFF_PID=$!

