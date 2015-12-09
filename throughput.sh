#!/bin/bash

IP=167772164
MESH_SIZE=$1
PORT=5001

ip2dec () {
    local a b c d ip=$@
    IFS=. read -r a b c d <<< "$ip"
    printf '%d\n' "$((a * 256 ** 3 + b * 256 ** 2 + c * 256 + d))"
}

dec2ip () {
    local ip dec=$@
    for e in {3..0}
    do
        ((octet = dec / (256 ** e) ))
        ((dec -= octet * 256 ** e))
        ip+=$delim$octet
        delim=.
    done
    printf '%s\n' "$ip"
}

netperf_temp () {
    BW=$(netperf -l 120 -H $1 -p $2 -t TCP_STREAM -P 0 -v 0 -D 1 -- -m 1436)
    echo $3 $BW
}

SELF_IP=$(/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}')
SELF_IP_N=$(ip2dec $SELF_IP)
SELF_ID=$((SELF_IP_N-IP))

for i in {0..9}
do
    if [ $i -eq $SELF_ID ]
    then
        echo Back
        continue
    fi
    FILE_NAME="out$(printf '%03d' $i)"
    IP_ADD=$(dec2ip $((IP+i)))
    netperf_temp $IP_ADD $PORT $i > $FILE_NAME &
done
