#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: ./checkpoint_send directoryname interval trial dest_ips"
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
MACHINE="src"
trap ctrl_c INT

# on ctrc_c, kill test_p2p processes
function ctrl_c() {
    pkill -9 test_p2p
    exit 0
}

if [ "$#" -ge 2 ]; then
    INTERVAL=$2
fi

if [ "$#" -ge 3 ]; then
    TRIAL=$3
fi

cp -rf $directoryname $directoryname$movefolder
echo "TRIAL $TRIAL" >> logtime.txt

START=$(date +'%Y-%m-%d %H:%M:%S.%3N')
echo "compress start: $START" >> logtime.txt
tar -cvzf $directoryname$movefolder.tar.gz $directoryname$movefolder
END=$(date +'%Y-%m-%d %H:%M:%S.%3N')
echo "compress end: $END" >> logtime.txt
rm -rf $directoryname$movefolder

echo "make torrent start: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> logtime.txt
$P2P_DIR/make_torrent -t $directoryname.torrent -f $directoryname$movefolder.tar.gz -s 256

for i in "${@:4}"
do
    scp -i $KEY $directoryname.torrent $USERNAME@$i:$DEST_DIR
done

echo "test p2p start: $(date +'%Y-%m-%d %H:%M:%S.%N')" >> logtime.txt
$P2P_DIR/test_p2p -t $directoryname.torrent -l . &

while true; do
    echo "still waiting"
    sleep 10
done
