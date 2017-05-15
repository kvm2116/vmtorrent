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
PEERFILE="/home/bw2387/peers.txt"

trap ctrl_c INT

# on ctrl_c, kill test_p2p processes
function ctrl_c() {
    pkill -9 test_p2p
    exit 0
}

if [ "$#" -ge 3 ]; then
    BYTES=$3
fi
if [ "$#" -ge 4 ]; then
    TRIAL=$4
fi
if [ "$#" -ge 5 ]; then
    MACHINE=$5
fi

rm -rf $directoryname
rm -rf $directoryname$movefolder
rm $directoryname$movefolder.tar.gz
rm -rf $directoryname.torrent

while [ ! -s $directoryname.torrent ]; do
    
    sleep 0.001
done

sleep 3

echo "TRIAL $TRIAL" >> log_testp2p_$MACHINE.txt
echo "test p2p start: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> log_testp2p_$MACHINE.txt
$P2P_DIR/test_p2p -t $directoryname.torrent -e $PEERFILE -l . -s $SRC_IP &
	
while [ ! -s $directoryname$movefolder.tar.gz ]; do
    sleep 0.001
done

CHECK=$(du -b $directoryname$movefolder.tar.gz  | cut -f1)
while [ "$CHECK" -lt "$BYTES" ]; do
    sleep 0.001
    CHECK=$(du -b $directoryname$movefolder.tar.gz | cut -f1)
    #echo "waiting for tar.gz"
done
        
echo "tar file received: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> log_testp2p_$MACHINE.txt
if [ -s $directoryname$movefolder.tar.gz ]; then
            
    echo "decompress start: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> log_testp2p_$MACHINE.txt
    tar -xvzf $directoryname$movefolder.tar.gz
    echo "decompress end: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> log_testp2p_$MACHINE.txt
    cp -a $directoryname$movefolder/. $directoryname
fi

#pkill -9 test_p2p
