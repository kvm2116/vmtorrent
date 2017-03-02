#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: ./checkpoint_send directoryname interval"
fi

directoryname=$1
movefolder="tomove"
INTERVAL=5
LAST_TIME=0
DEST_IP=$3
DEST_DIR=/home/bw2387/

echo "$directoryname"

if [ "$#" -eq 2 ]; then
    INTERVAL=$2
fi

while true; do
    
    if [ $LAST_TIME -eq 0 ]; then
        cp -rf $directoryname $directoryname$movefolder
        LAST_TIME=$(date +%s)
        echo $LAST_TIME
    else
        echo "not first time"
        result=$(find $directoryname -newermt "-$INTERVAL seconds")
        if [[ $result ]]; then
            mkdir $directoryname$movefolder
            find $directoryname -newermt "-$INTERVAL seconds" | xargs cp -t $directoryname$movefolder       
        fi
    fi

    if [ -d "$directoryname$movefolder" ]; then

        tar -cvzf $directoryname$movefolder.tar.gz $directoryname$movefolder
        rm -rf $directoryname$movefolder

        # kill old test_p2p
        pkill -9 test_p2p

        ./make_torrent -t $directoryname.torrent -f $directoryname$movefolder.tar.gz -s 256
        scp $directoryname.torrent bw2387@$DEST_IP:$DEST_DIR

        ./test_p2p -t $directoryname.torrent -l . &

        sleep $INTERVAL

#        pkill -9 test_p2p
    else
        sleep $INTERVAL
    fi

done
