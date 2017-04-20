COMMAND=$1
TRIAL=$2
USER="bw2387"
KEY="/home/bw2387/.ssh/vmtorrent_key"
IPstring=`cat dest_ips2.txt`
IPS=' ' read -r -a array <<< "$IPstring"
SRC_IP="10.142.0.2"

echo "SENDING DEST IPS"
MACHINE_NUM=1
for element in "${array[@]}"
do
    echo "$element"
    if [ "$COMMAND" -eq "1" ]
    then
        ssh -i $KEY -f $USER@$element "sh -c 'export LD_LIBRARY_PATH=\"/home/bw2387/vmtorrent/p2pfs/libtorrent/src/.libs:/home/bw2387/vmtorrent/p2pfs/boost/install/lib\"; cd ~/test_scripts; nohup ./../vmtorrent/p2pfs/p2p/testp2p_receive.sh checkpoint1 $SRC_IP 5 $TRIAL $MACHINE_NUM > receive_p2p.out 2> receive_p2p.err < /dev/null &'"
    elif [ "$COMMAND" -eq "2" ]
    then
        ssh -i $KEY -f $USER@$element "sh -c 'nohup ./vmtorrent/p2pfs/p2p/receive_scp.sh $TRIAL checkpoint1_tar 2131975 scp_tar_logfile_$MACHINE_NUM.txt > receive_tar.out 2> receive_tar.err < /dev/null &'"
    elif [ "$COMMAND" -eq "3" ]
    then
        ssh -i $KEY -f $USER@$element "sh -c 'nohup ./vmtorrent/p2pfs/p2p/receive_scp.sh $TRIAL checkpoint1 186460983 scp_logfile_$MACHINE_NUM.txt > receive_rev.out 2> receive_rev.err < /dev/null &'"
    fi
    let "MACHINE_NUM=MACHINE_NUM+1"
done

echo "RUNNING on SRC"
if [ "$COMMAND" -eq "1" ]
then
    cd test_scripts;
    nohup ./../vmtorrent/p2pfs/p2p/testp2p_send.sh checkpoint1 5 $TRIAL $IPstring > send_p2p.out 2> send_p2p.err < /dev/null  &
    echo "KILLING TEST_P2P"
    sleep 30
    pkill test_p2p
elif [ "$COMMAND" -eq "2" ]
then
    cd test_scripts;
    nohup python ../vmtorrent/p2pfs/p2p/send_tar_scp.py checkpoint1 ~/checkpoint1_tar $TRIAL $IPstring > send_tar.out 2> send_tar.err < /dev/null &
elif [ "$COMMAND" -eq "3" ]
then
    cd test_scripts;
    nohup python ../vmtorrent/p2pfs/p2p/send_scp.py checkpoint1 ~/ $TRIAL $IPstring > send_rec.out 2> send_rec.err < /dev/null &
fi


