#!/bin/bash

SELF_NAME=$(hostname)
SELF_ID=${SELF_NAME:1:1}

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

