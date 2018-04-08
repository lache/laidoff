#!/bin/bash

pushd `pwd`
cd services/db
../../bin/br-server db >> db.log 2>&1 &
DB_PID=$!
popd
sleep 1

pushd `pwd`
cd services/match
../../bin/br-server match >> match.log 2>&1 &
MATCH_PID=$!
popd
sleep 1

pushd `pwd`
cd services/rank
../../bin/br-server rank >> rank.log 2>&1 &
RANK_PID=$!
popd
sleep 1

pushd `pwd`
cd services/reward
../../bin/br-server reward >> reward.log 2>&1 &
REWARD_PID=$!
popd
sleep 1

pushd `pwd`
cd services/push
../../bin/br-server push >> push.log 2>&1 &
PUSH_PID=$!
popd
sleep 1

pushd `pwd`
cd services/battle
../../bin/laidoff-server >> laidoff.log 2>&1
# Never reach here...
LAIDOFF_PID=$!
popd

