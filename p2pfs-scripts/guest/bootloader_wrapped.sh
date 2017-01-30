#!/bin/bash

### UNCOMMENT TO ENABLE TIMESYNCING
#START_NTP=`date +%s.%N` 
#echo -e "BOOTLOADER STARTING, WAITING FOR TIMESERVER $NTPSERVER"
#while [ -z "$RES" ]; do
#    END_NTP=`date +%s.%N` 
#    RES=`sudo ntpdate $NTPSERVER`
#    RES=`echo $RES | grep "sec"`
#done
#echo "bootloader_wrapped:RES=$RES"
#NTP_DELAY=`echo "bootloader_wrapped:$END_NTP - $START_NTP" | bc`
start_time  # DON'T COMMENT OUT, EVEN IF TIMESYNCING DISABLED!
#time_elapsed_diff; echo -e "SYNCED CLOCK W/ $NTPSERVER, DELAY=$NTP_DELAY"
#echo "bootloader_wrapped:RES=$RES" >> $HOST_GUEST_DIR/delay.log
#echo -e "SYNCED CLOCK W/ $NTPSERVER, DELAY=$NTP_DELAY" >> $HOST_GUEST_DIR/delay.log

IP=`ifconfig | grep inet | head -1 | sed -e "s/:/ /" | awk '{print $3}'`
ETH=`ifconfig | grep -A 1 eth | head -1 | awk '{print $1}'` 
time_elapsed_diff; echo "bootloader_wrapped:IP=$IP:ETH=$ETH" 
HOST_IP=`echo $IP | sed -e "s/\./ /g" | awk '{print $1"."$2"."$3".1"}'`

if [ -z "$HOST_IP" ]
then
    time_elapsed_diff; echo "bootloader_wrapped:NO HOST_IP, giving up" 
    logsync
    sudo shutdown -h now
else
    #LOGIN_ID="reich"
    #DATASTORE="$LOGIN_ID@$HOST_IP:/local/reich/guests"
    time_elapsed_diff; echo "bootloader_wrapped:HOST_IP=$HOST_IP" 
    #logsync
 
    # get params and scripts from webserver
#    wget http://$HOST_IP:8000/$PARAMS 
#    if [ ! -f $PARAMS ] ; then
#	time_elapsed_diff; echo "bootloader_wrapped:$PARAMS failed to download"
#	rm index.html
#	logsync
#	exit 1	
#    fi
#    time_elapsed_diff; echo "bootloader_wrapped:$PARAMS file obtained" 

    PARAMS=$HOST_GUEST_SCRIPT_DIR/$PARAMS
    EXPERIMENT_DIRECTORY=`sed -n '1p' $PARAMS | awk '{print $1}'`
    #EXPERIMENT_DIRECTORY=$HOST_GUEST_DIR/$EXPERIMENT_DIRECTORY
    SCRIPT=`sed -n '2p' $PARAMS | awk '{print $1}'`
    POSTSCRIPT_ACTION=`sed -n '3p' $PARAMS | awk '{print $1}'`
    time_elapsed_diff; echo "bootloader_wrapped:EXPERIMENT_DIRECTORY=$EXPERIMENT_DIRECTORY"
    time_elapsed_diff; echo "bootloader_wrapped:SCRIPT=$SCRIPT" 
    time_elapsed_diff; echo "bootloader_wrapped:POSTSCRIPT_ACTION=$POSTSCRIPT_ACTION"     
    logsync

    # get script file from webserver
#    wget http://$HOST_IP:8000/$SCRIPT 
#    if [ ! -f $SCRIPT ]; then
#	time_elapsed_diff; echo "bootloader_wrapped:$SCRIPT failed to download" 
#	rm index.html
#	logsync
#	exit 1	
#    fi
#    time_elapsed_diff; echo "bootloader_wrapped:$SCRIPT obtained"  
#    logsync

    SCRIPT=$HOST_GUEST_SCRIPT_DIR/$SCRIPT
    chmod +x $SCRIPT
    time_elapsed_diff; echo "bootloader_wrapped:$SCRIPT starting " 
    source $SCRIPT
    time_elapsed_diff; echo "bootloader_wrapped:$SCRIPT done " 
    logsync
    
    #time_elapsed; echo "bootloader_wrapped:removing $SCRIPT $PARAMS " 
    #rm $SCRIPT $PARAMS

    case "$POSTSCRIPT_ACTION" in 
	shutdown)
	    time_elapsed_diff; echo "bootloader_wrapped:shutting down" 
	    date
	    logsync
	    sudo shutdown -h now
	    ;;
	reboot)
	    time_elapsed_diff; echo "bootloader_wrapped:rebooting" 
	    date
	    logsync
	    sudo reboot
	    ;;
	*)
	    date
	    logsync
	    ;;
    esac

fi
