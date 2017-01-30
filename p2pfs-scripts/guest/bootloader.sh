#!/bin/bash

HOST_GUEST_DIR="/mnt/hgfs/guest"
HOST_GUEST_SCRIPT_DIR="$HOST_GUEST_DIR/scripts"
GUESTLOG="$HOST_GUEST_DIR/newest/guest.log"
NTPSERVER="timex.cs.columbia.edu"
PARAMS="params.txt"

start_time()
{
    START_T=`date +%s.%N` 
    LAST_T=$START_T
    printf "%.3f ----- -----:" $START_T
    echo -e "\tSTART TIME"
}

time_elapsed()
{
    CUR_T=`date +%s.%N`
    TOTAL_ELAPSED=`echo "$CUR_T - $START_T" | bc`
    echo -n `printf "%.3f %.3f -----:" $CUR_T $TOTAL_ELAPSED`
    echo -en "\t"
}

time_elapsed_diff()
{
    CUR_T=`date +%s.%N`
    TOTAL_ELAPSED=`echo "$CUR_T - $START_T" | bc`
    LAST_ELAPSED=`echo "$CUR_T - $LAST_T" | bc`
    LAST_T=$CUR_T
    echo -n `printf "%.3f %.3f %.3f:" $CUR_T $TOTAL_ELAPSED $LAST_ELAPSED`
    echo -en "\t"
}

logsync()
{
    sync
#    rsync $GUESTLOG $DATASTORE
#    time_elapsed_diff; echo "finished sync to $DATASTORE" 
}

echo "starting bootloader.sh" > guest.log
echo "checking for $HOST_GUEST_SCRIPT_DIR/bootloader_wrapped.sh" >> guest.log
counter=0;
while [ ! -e "$HOST_GUEST_SCRIPT_DIR/bootloader_wrapped.sh" ]; do
    echo "$counter: waiting for $HOST_GUEST_SCRIPT_DIR/bootloader_wrapped.sh"  >> guest.log
    let "counter += 1";
    sleep 1
done
echo "bootloader_wrapped.sh available, wait was $counter seconds" >> guest.log
logsync

echo "checking for ${GUESTLOG%/*}" >> guest.log
counter=0;
while [ ! -d "${GUESTLOG%/*}" ]; do
    echo "$counter: waiting for ${GUESTLOG%/*}"  >> guest.log
    let "counter += 1";
    sleep 1
done
echo "bootloader_wrapped.sh available, wait was $counter seconds" >> guest.log
logsync
cat guest.log > $GUESTLOG
source $HOST_GUEST_SCRIPT_DIR/bootloader_wrapped.sh &>> $GUESTLOG
echo "done" >> guest.log