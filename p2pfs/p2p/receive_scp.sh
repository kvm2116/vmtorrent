TRIAL=$1
DIR=$2
SIZE=$3
LOGFILE=$4

rm -rf $DIR
mkdir $DIR

CHECK=$(du -b $DIR  | cut -f1)    
echo "TRIAL $TRIAL" >> $LOGFILE

while [ "$CHECK" -lt "$SIZE" ]
do
    sleep 0.001
    CHECK=$(du -b $DIR  | cut -f1)    
done

echo $CHECK
echo "DONE"
echo "entire folder received: $(date +'%Y-%m-%d %H:%M:%S.%3N')" >> $LOGFILE

