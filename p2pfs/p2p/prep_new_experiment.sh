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
    ssh -i $KEY -f $USER@$element "sh -c 'rm scp_logfile_$MACHINE_NUM.txt scp_tar_logfile_$MACHINE_NUM.txt; cd test_scripts; rm log_testp2p_$MACHINE_NUM.txt'"
    let "MACHINE_NUM=MACHINE_NUM+1"
done

echo "RUNNING on SRC"
cd test_scripts;
rm *.err;
rm *.out;
rm logtime.txt scp_logtime.txt scp_tar_logtime.txt;

