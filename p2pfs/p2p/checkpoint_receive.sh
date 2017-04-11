#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: ./checkpoint_receive directoryname src_ip interval"
fi

directoryname=$1
movefolder="tomove"
INTERVAL=5
LAST_TIME=0
SRC_IP=$2
P2P_DIR="/home/bw2387/vmtorrent/p2pfs/p2p"
TRIAL=1

trap ctrl_c INT

# on ctrl_c, kill test_p2p processes
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
	
        LAST_TIME=$(date +%s)
        echo "first iteration"
        echo "TRIAL $TRIAL" >> logtime.txt
        echo "test p2p start: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> logtime.txt
        $P2P_DIR/test_p2p -t $directoryname.torrent -l . -s $SRC_IP &
	
        while [ ! -s $directoryname$movefolder.tar.gz ]; do
            sleep 0.01
            #echo "waiting for tar.gz"
        done
        
        echo "tar file received: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> logtime.txt
        if [ -s $directoryname$movefolder.tar.gz ]; then
            
            echo "decompress start: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> logtime.txt
            tar -xvzf $directoryname$movefolder.tar.gz
            echo "decompress end: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> logtime.txt
            cp -a $directoryname$movefolder/. $directoryname
        fi
    else
        echo "next iteration"   
        LAST_TIME=$(date +%s)
        result=$(find . -name $directoryname.torrent -newermt "-$INTERVAL seconds")
        if [[ $result ]]; then
            pkill -9 test_p2p
            rm -rf $directoryname$movefolder
            rm $directoryname$movefolder.tar.gz
            $P2P_DIR/test_p2p -t $directoryname.torrent -l . -s $SRC_IP &
            while [ ! -s $directoryname$movefolder.tar.gz ]; do
                sleep 0.1
                echo "waiting for tar.gz"
            done
            if [ -s $directoryname$movefolder.tar.gz ]; then
                tar -xvzf $directoryname$movefolder.tar.gz
                cp -a $directoryname$movefolder/. $directoryname
            fi
        fi
    fi

    cur_time=$(date +%s)
    let sleep_time="$INTERVAL-($cur_time-$LAST_TIME)"
    sleep $sleep_time
done
