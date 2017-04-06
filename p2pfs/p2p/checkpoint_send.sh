#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: ./checkpoint_send directoryname dest_ip interval"
fi

directoryname=$1
movefolder="tomove"
INTERVAL=5
LAST_TIME=0
DEST_IP=$2
DEST_DIR="/home/bw2387/test_scripts"
USERNAME="bw2387"
P2P_DIR="/home/bw2387/vmtorrent/p2pfs/p2p"
KEY="/home/bw2387/.ssh/vmtorrent_key"
TRIAL=1
trap ctrl_c INT

# on ctrc_c, kill test_p2p processes
function ctrl_c() {
    pkill -9 test_p2p
    exit 0
}

if [ "$#" -ge 3 ]; then
    INTERVAL=$3
fi

if [ "$#" -ge 4 ]; then
    TRIAL=$4
fi

while true; do
    
    if [ $LAST_TIME -eq 0 ]; then
        cp -rf $directoryname $directoryname$movefolder
        LAST_TIME=$(date +%s)
        echo "first iteration"
    else
        echo "next iteration"
        LAST_TIME=$(date +%s)
        result=$(find $directoryname -newermt "-$INTERVAL seconds")
        if [[ $result ]]; then
            mkdir $directoryname$movefolder
            resultdir=$(find $directoryname -mindepth 1 -type d -newermt "-$INTERVAL seconds")
            if [[ $resultdir ]]; then
                find $directoryname -mindepth 1 -type d -newermt "-$INTERVAL seconds" | xargs cp -r -t $directoryname$movefolder
            fi
            resultfiles=$(find $directoryname -maxdepth 1 -type f -newermt "-$INTERVAL seconds")
            if [[ $resultfiles ]]; then
                find $directoryname -maxdepth 1 -type f -newermt "-$INTERVAL seconds" | xargs cp -t $directoryname$movefolder
            fi

        fi
    fi

    if [ -d "$directoryname$movefolder" ]; then

        echo "TRIAL $TRIAL" >> logtime.txt
        START=$(date +'%Y-%m-%d %H:%M:%S.%3N')
        echo "compress start: $START" >> logtime.txt
        tar -cvzf $directoryname$movefolder.tar.gz $directoryname$movefolder
        END=$(date +'%Y-%m-%d %H:%M:%S.%3N')
        echo "compress end: $END" >> logtime.txt
        rm -rf $directoryname$movefolder

        # kill old test_p2p
        pkill -9 test_p2p

        echo "make torrent start: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> logtime.txt
        $P2P_DIR/make_torrent -t $directoryname.torrent -f $directoryname$movefolder.tar.gz -s 256
        scp -i $KEY $directoryname.torrent $USERNAME@$DEST_IP:$DEST_DIR
        
        echo "test p2p start: $(date +'%Y-%m-%d %H:%M:%S.%N')" >> logtime.txt
        $P2P_DIR/test_p2p -t $directoryname.torrent -l . &
    fi
        
    cur_time=$(date +%s)
    let sleep_time="$INTERVAL-($cur_time-$LAST_TIME)"
    sleep $sleep_time

done
