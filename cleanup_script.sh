#!/bin/bash

IP=167772164

ip2dec () {
    local a b c d ip=$@
    IFS=. read -r a b c d <<< "$ip"
    printf '%d\n' "$((a * 256 ** 3 + b * 256 ** 2 + c * 256 + d))"
}

SELF_IP=$(/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}')
SELF_IP_N=$(ip2dec $SELF_IP)
SELF_ID=$((SELF_IP_N-IP))

echo $SELF_ID


for i in {0..9}
do
    if [ $i -eq $SELF_ID ]
    then
        echo Back
        continue
    fi
    FILE_NAME="in$(printf '%03d' $i)"
    FILE_NAME_OUT="out$(printf '%03d' $i)"

    vim -s test.keys $FILE_NAME
    cat $FILE_NAME | awk '{print $3, $6}' > $FILE_NAME_OUT
done

