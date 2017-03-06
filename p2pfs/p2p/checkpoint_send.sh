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

trap ctrl_c INT

# on ctrc_c, kill test_p2p processes
function ctrl_c() {
    pkill -9 test_p2p
    exit 0
}

if [ "$#" -eq 3 ]; then
    INTERVAL=$3
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
            find $directoryname -type d -mindepth 1 -newermt "-$INTERVAL seconds" | xargs cp -r -t $directoryname$movefolder
            find $directoryname -type f -maxdepth 1 -newermt "-$INTERVAL seconds" | xargs cp -t $directoryname$movefolder

        fi
    fi

    if [ -d "$directoryname$movefolder" ]; then

        tar -cvzf $directoryname$movefolder.tar.gz $directoryname$movefolder
        rm -rf $directoryname$movefolder

        # kill old test_p2p
        pkill -9 test_p2p

        $P2P_DIR/make_torrent -t $directoryname.torrent -f $directoryname$movefolder.tar.gz -s 256
        scp $directoryname.torrent $USERNAME@$DEST_IP:$DEST_DIR

        $P2P_DIR/test_p2p -t $directoryname.torrent -l . &
    fi
        
    cur_time=$(date +%s)
    let sleep_time="$INTERVAL-($cur_time-$LAST_TIME)"
    sleep $sleep_time

done
