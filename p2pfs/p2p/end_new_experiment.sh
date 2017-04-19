EXP_NUM=$1
USER="bw2387"
KEY="/home/bw2387/.ssh/vmtorrent_key"
IPstring=`cat dest_ips2.txt`
IPS=' ' read -r -a array <<< "$IPstring"
SRC_IP="10.142.0.2"

mkdir test_data_$EXP_NUM;

echo "SENDING DEST IPS";
MACHINE_NUM=1
for element in "${array[@]}"
do
    echo "$element";
    scp -i $KEY $USER@$element:~/\{scp_logfile_$MACHINE_NUM.txt,scp_tar_logfile_$MACHINE_NUM.txt,test_scripts/log_testp2p_$MACHINE_NUM.txt\} test_data_$EXP_NUM;
    MACHINE_NUM=$MACHINE_NUM+1
done

echo "RUNNING on SRC"
cd test_scripts;
cp logtime.txt scp_logtime.txt scp_tar_logtime.txt ../test_data_$EXP_NUM;

