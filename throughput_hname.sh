#!/bin/bash

MESH_SIZE=$1
PORT=5001
SUFFIX='.exp.reactor.marmot.pdl.cmu.local'

netperf_temp () {
    BW=$(netperf -l 120 -H $1 -p $2 -t TCP_STREAM -P 0 -v 0 -D 1 -- -m 1436)
    echo $3 $BW
}

SELF_NAME=$(hostname)
SELF_ID=${SELF_NAME:1:1}

for i in {0..9}
do
    if [ $i -eq $SELF_ID ]
    then
        echo Back
        continue
    fi
    FILE_NAME="in$(printf '%03d' $i)"
    HOST_NAME="h$(printf $i)"$SUFFIX
    netperf_temp $HOST_NAME $PORT $i > $FILE_NAME &
done
