#!/bin/bash

cd db
../br/br-server db >> db.log 2>&1 &
DB_PID=$!
cd ..
sleep 1

cd match
../br/br-server match >> match.log 2>&1 &
MATCH_PID=$!
cd ..
sleep 1

cd rank
../br/br-server rank >> rank.log 2>&1 &
RANK_PID=$!
cd ..
sleep 1

cd reward
../br/br-server reward >> reward.log 2>&1 &
REWARD_PID=$!
cd ..
sleep 1

cd push
../br/br-server push >> push.log 2>&1 &
PUSH_PID=$!
cd ..
sleep 1

cd battle
./laidoff-server >> laidoff.log 2>&1
LAIDOFF_PID=$!

