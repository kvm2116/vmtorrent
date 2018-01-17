USER="mkunal"
# KEY="/home/bw2387/.ssh/vmtorrent_key"
IPstring=`cat ../vmtorrent/p2pfs/p2p/dest_ips.txt`
IPS=' ' read -r -a array <<< "$IPstring"
# SRC_IP="10.142.0.2"

echo "SENDING DEST IPS"
MACHINE_NUM=1
for element in "${array[@]}"
do
    echo "$element"
    ssh -f $USER@$element "sh -c 'rm go.tar.gz *.err *.out scp_logfile_$MACHINE_NUM.txt rsync_logfile_$MACHINE_NUM.txt scp_tar_logfile_$MACHINE_NUM.txt; cd test_scripts; rm log_testp2p_$MACHINE_NUM.txt'"
    let "MACHINE_NUM=MACHINE_NUM+1"
done

echo "RUNNING on SRC"
# cd test_scripts;
rm *.err;
rm *.out;
rm logtime.txt scp_logtime.txt rsync_logtime.txt scp_tar_logtime.txt;

