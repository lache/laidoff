#!/bin/bash

docker run -d \
	-p 10288:10288/udp \
	-p 18080:18080 \
	-p 19856:19856 \
	-p 20182:20182 \
	-e "BATTLE_HOST=`dig +short myip.opendns.com @resolver1.opendns.com`" \
	-v '/etc/ssl/certs/ca-certificates.crt:/etc/ssl/certs/ca-certificates.crt' \
	br

