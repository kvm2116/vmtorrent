#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: ./checkpoint_send directoryname"
fi

directoryname=$1

echo "$directoryname"

tar -cvzf $directoryname.tar.gz $directoryname

./make_torrent -t $directoryname.torrent -f $directoryname.tar.gz -s 256

./test_p2p -t $directoryname.torrent -l . &

sleep 2m

pkill -9 test_p2p 
