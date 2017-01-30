#! /usr/bin/env python

import sys
import os
import re
from datetime import datetime, date, time
import string
import math
from operator import itemgetter
from itertools import groupby


### HELPER METHODS AND CLASSES

class mdict(dict):

    def __setitem__(self, key, value):
        """add the given value to the list of values for this key"""
        self.setdefault(key, []).append(value)

import traceback
def formatExceptionInfo(maxTBlevel=5):
    cla, exc, trbk = sys.exc_info()
    excName = cla.__name__
    try:
        excArgs = exc.__dict__["args"]
    except KeyError:
        excArgs = "<no args>"
        excTb = traceback.format_tb(trbk, maxTBlevel)
        return (excName, excArgs, excTb)
    

# THIS SCRIPT READS THE PROFILE AND P2PM.LOG
# IT COMPARES THE TWO AND GENERATES STATISTICS
def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False

def is_integer(s):
    try:
        int(s)
        return True
    except ValueError:
        return False

def is_valid_timestamp(s):
    m = re.search('^\d{10}\.\d+$', s)
    if m is None:
        return False
    else:
        return True


def enter_in_keyvals(item,keyvals):
    global blocks_per_piece
    m = re.search('=',item)
    if m is None:
        keyvals['action'] += " " + item
    else:
        keyval = item.split('=')
        if len(keyval) == 2:
            if is_integer(keyval[1]):
                if keyval[0] == 'block':
                    keyvals['piece'] = int(keyval[1])/blocks_per_piece
                keyvals[keyval[0]] = int(keyval[1])
            elif is_number(keyval[1]):
                keyvals[keyval[0]] = float(keyval[1])
            else:
                keyvals[keyval[0]] = keyval[1]
        else:
            keyvals['action'] += " " +  item


def parse_msg(msg):
    keyvals = { 'action' : ''}
    token=msg.split(",")
    token = map(lambda x: x.strip(), token)
    map(lambda x: enter_in_keyvals(x,keyvals), token)
    keyvals['action'] = keyvals['action'].lstrip()
    return keyvals


# END HELPER METHODS AND CLASSES


### GLOBAL VARIABLES

start_time = 0
blocks_per_piece=16
last_profile_request=0
time_event=mdict()
block_events={}
timeline_event={}
mutex_event={}
piece_events={}
thread_events={}
thread_alias={}
fs_events=[]
error = open('errorlines.dbg', 'w')

### END GLOBAL VARIABLES


### LOG PARSING

def parse_log(name):
    global start_time
    global blocks_per_piece
    global last_profile_request
    thread_alias_counter = 0
    parsed_events={}
    numgoodlines=0
    numerrorlines=0
    profile_rank_counter=0
    te_names = [ 'START', 'START P2PM', 'P2PM STARTED', 'ADDING P2PM PROFILE', 'P2PM PROFILE ADDED', 'START FUSE' ,'CLOSE', 'obtained handle' , 'DONE FUSE', 'DONE P2PM', 'DONE' ]
    firstneed = None
    infile = open(name,'r')
    for line in infile:
        msg = None
        cols = line.split(":")

        if not is_valid_timestamp(cols[0]):
            error.write(line)
            numerrorlines += 1
            #print "invalid timestamp: %s" % (cols)
            continue
        
        time = float(cols[0])

        # handle ssh logfiles
        if name=='exec_trial.log' and len(cols) == 6: 
            m = re.search('starting VM', line)
            if m is None:
                pass
            else:
                start_time = float(cols[0])
                #print "GOT START TIME FROM exec_trials.log %f" % (start_time)
 
            continue
            
        # skip improperly formated files
        elif len(cols) < 6 or \
                not is_integer(cols[3+1]):
            error.write(line)
            numerrorlines += 1
            #print "improperly formatted line: %s" % (cols)
            continue

        else: 
            numgoodlines += 1
            thread = cols[1]
            
            # map thread id to sequential number starting at 0
            # not ideal b/c main threads for p2pm and fs will be numbered
            # differently depending on which is parsed first
            if thread not in thread_alias:
                thread_alias[thread] = thread_alias_counter
                thread_alias_counter += 1
                thread = thread_alias[thread]
                
            # w/ nice
            nice = int(cols[2])
            srcfile = cols[2+1]
            srcline = int(cols[3+1])
            fnc = cols[4+1]
            try:        
                msg = cols[5+1].strip()
            except IndexError:
                #print "IndexError on: %s" % (cols)
                continue
    
        if msg is None:
            #print "msg is None: %s" % (cols)
            continue

        event = parse_msg(msg)
        event['time'] = time
        event['thread'] = thread
        event['nice'] = nice
        event['srcfile'] = srcfile
        event['srcline'] = srcline
        event['fnc'] = fnc
        event['msg'] = msg

            
        # build timeline
        if 'fnc' in event and event['fnc'] == 'exec_trial':
                timeline_event[time] = event
        elif 'action' in event:
            if (event['action'] in te_names or \
                    (event['fnc'] == 'bindfs_open' and event['action'] == 'done') or \
                    (event['fnc'] == 'second_tick' and False)
                ):
                timeline_event[time] = event
            elif firstneed is None and event['action'] == 'need': 
                firstneed = time
                timeline_event[time] = event
            
        # collect mutex delays
        if 'delay' in event and event['action'] == 'obtained':
            mutex_event[event['delay']] = event

        # collect piece events
        if 'piece' in event:
            if event['action'] == 'add_to_profile':
                event['rank'] = profile_rank_counter
                profile_rank_counter += 1
            try: 
                piece_events[event['piece']].append(event)
            except KeyError:
                piece_events[event['piece']] = [event]
                
        # collect fs events
        if srcfile == 'bindfs.c' or srcfile == 'blocks.c':
            fs_events.append(event)

        # key by time
        try:
            time_event[time].append(event)
        except:
            time_event[time] = [event]

        if fnc=='set_piece_priority' and int(event['priority']) == 6:
            last_profile_request=time

        if blocks_per_piece < 0 and \
                'blocks_per_piece' in event:
            blocks_per_piece = int(event['blocks_per_piece'])
            #print "blocks_per_piece=%d" % blocks_per_piece

        # key by thread
        try: 
            thread_events[thread][time] = event
        except KeyError:
            thread_events[thread] = {time : event}


        # key by block
        if 'block' in event and is_integer(event['block']):
            try: 
                block_events[int(event['block'])][time] = event
            except KeyError: 
                block_events[int(event['block'])] = mdict()
                block_events[int(event['block'])][time] = event

        elif 'piece' in event:
            if is_integer(event['piece']):
                first_block=int(event['piece'])*blocks_per_piece
                blocks = range(first_block,first_block+blocks_per_piece)
                for block in blocks:
                    event['block'] = block
                    try:                   
                        block_events[block][time] = event
                    except KeyError: 
                        block_events[int(event['block'])] = mdict()
                        block_events[block][time] = event
                    
    infile.close()
    print "file=%s, correct=%d, errors=%d" % (name, numgoodlines, numerrorlines)

### END LOG PARSING


### ANALYSIS

accessed_blocks = {}
accessed_pieces = {}
def build_fs_stats():
    global start_time
    global blocks_per_piece
    f=open("fs.block.accesses.tab",'w')
    g=open("fs.delay.timeline.tab",'w')
    h=open("fs.piece.accesses.tab",'w')
    f.write("#TIME\t\tBLOCK\t\tTYPE\n")
    g.write("#TIME\t\tDELAY\n")
    h.write("#TIME\t\tPIECE\t\tTYPE\n")
    #print len(sorted(fs_events, key=itemgetter('time')))
    for fs_event in sorted(fs_events, key=itemgetter('time')):

        if fs_event['fnc'] == 'bh_bring_block' and fs_event['action'] == 'start':
            accessed_block=fs_event['block']
            if not accessed_blocks.has_key(accessed_block):
                accessed_blocks[accessed_block] = fs_event['time']
                f.write("%f\t%8d\t%s\n" % (fs_event['time']-start_time,accessed_block,fs_event['type']))
                accessed_piece=math.floor(accessed_block/blocks_per_piece)
                if not accessed_pieces.has_key(accessed_piece):
                    accessed_pieces[accessed_piece] = fs_event['time']
                    h.write("%f\t%8d\t%s\n" % (fs_event['time']-start_time,accessed_piece,fs_event['type']))
                

        if fs_event['action'] == 'start I/O stall' or fs_event['action'] == 'end I/O stall':
            try:
                g.write("%f\t%f\n" % (fs_event['time']-start_time,fs_event['CUMULATIVE_STALL']))
            except KeyError:
                pass
                

    f.close()
    g.close()
    h.close()
    

piece_stats=[]
def build_piece_stats():
    global start_time

    for (piece, events) in sorted(piece_events.items(),key=itemgetter(0)):
        piece_stat={ 'piece' : piece, 'profiled' : False, 'priority' : 0 }
        libtorrent_delay = -1
        piece_stat['fs_req'] = start_time - 1
        for event in sorted(events, key=itemgetter('time')):
            if event['fnc'] == 'bh_bring_block' and event['action'] == 'start':
                if piece_stat['fs_req'] < start_time:
                    piece_stat['fs_req'] = event['time']
            if event['action'] == 'add_to_profile':
                piece_stat['profiled'] = True
                piece_stat['min_access_time'] = event['min_access_time'] 
                piece_stat['mean_access_time'] = event['mean_access_time'] 
                piece_stat['frequency'] = event['frequency'] 
                piece_stat['rank'] = event['rank'] 
            if event['srcfile'] == 'piece_picker.cpp' and event['fnc'] == 'set_piece_priority':
                piece_stat['set_piece_priority_time'] = event['time']
                piece_stat['priority'] = int(event['priority'])
            if  event['srcfile'] == 'peer_connection.cpp' and event['fnc'] == 'incoming_piece':
                piece_stat['incoming_piece'] = event['time']
            if 'delay' in event and event['action'] == 'filled request':
                piece_stat['p2pm'] = float(event['delay'])

        if 'set_piece_priority_time' in piece_stat and 'incoming_piece' in piece_stat:
            piece_stat['libtorrent'] = piece_stat['incoming_piece'] - piece_stat['set_piece_priority_time']
        else:
            piece_stat['libtorrent'] = 0;

        if piece_stat['fs_req'] > start_time and 'incoming_piece' in piece_stat:
            piece_stat['early'] = piece_stat['fs_req'] - piece_stat['incoming_piece']
        else:
            piece_stat['early'] = 0

        if piece_stat['profiled']:
            if piece_stat['priority'] == 7:
                piece_stat['profile_outcome'] = 'true_positive_miss'
            else:
                if piece in accessed_pieces:
                    piece_stat['profile_outcome'] = 'true_positive'
                else:
                    piece_stat['profile_outcome'] = 'false_positive'
        else:
            piece_stat['profile_outcome'] = 'false_negative'
        piece_stats.append(piece_stat)


    f=open("network.delay.tab",'w')
    g=open("network.delay.timeline.tab",'w')
    h=open("network.effective_rate.tab",'w')
    f.write("#PIECE\tPROFILE_OUTCOME\t\tATTRIB\t\tEARLY\t\tFS_REQ_TIME\tREQUEST_OFF\tRECEIVE_OFF\t100mb_TIME\tMIN_TIME\tMEAN_TIME\tFREQ\t\tRANK\n")
    g.write("#TIME\t\tDELAY\n")
    g.write("%f\t%f\n" % (0, 0))
    end_marker=0
    network_delay=0
    aggregate_piece_delay=0
    counter=0
    min_access_max_gap=0
    mean_access_max_gap=0
    access_counter=0
    download_rate = { start_time : 0 }
    for piece_stat in sorted([p for p in piece_stats if 'set_piece_priority_time' in p and 'incoming_piece' in p ], key=itemgetter('set_piece_priority_time')):
        try:
            download_rate[int(round(piece_stat['incoming_piece']))] += 0.25
        except:
            download_rate[int(round(piece_stat['incoming_piece']))] = 0.25

    h.write("#TIME\t\tSEC\tDL-RATE\n")
    for (t,rate) in sorted(download_rate.items(), key=itemgetter(0)):
        h.write("%d\t%d\t%f\n" % (t,t-start_time,rate))
    h.close()

    for piece_stat in sorted([p for p in piece_stats if 'set_piece_priority_time' in p], key=itemgetter('fs_req')):

        if piece_stat['priority'] == 7:
            if 'incoming_piece' in piece_stat:
                if piece_stat['incoming_piece'] < end_marker:
                    piece_stat['attributed_delay'] = 0
                elif piece_stat['set_piece_priority_time'] < end_marker:
                    piece_stat['attributed_delay'] = piece_stat['incoming_piece'] - end_marker
                else:
                    piece_stat['attributed_delay'] = piece_stat['incoming_piece'] - piece_stat['set_piece_priority_time']

                piece_stat['attributed_delay'] = max(piece_stat['attributed_delay'],0)

                end_marker = max(end_marker, piece_stat['incoming_piece'])
                aggregate_piece_delay += piece_stat['libtorrent']
            else:
                piece_stat['attributed_delay'] = -1

        elif piece_stat['profile_outcome'] == 'false_positive':
            piece_stat['attributed_delay'] = float(1)/42

        else:
            piece_stat['attributed_delay'] = 0


        if piece_stat['attributed_delay'] > 0:
            network_delay += piece_stat['attributed_delay']
        g.write("%f\t%f\n" % (end_marker - start_time, network_delay))


        try:
            set_piece_priority_offset = piece_stat['set_piece_priority_time'] - start_time 
        except KeyError:
            set_piece_priority_offset = -1 

        try:
            incoming_piece_offset = piece_stat['incoming_piece'] - start_time
        except KeyError:
            incoming_piece_offset = -1

        try:
            if piece_stat['fs_req'] > start_time:
                access_rank = \
                    len([p for p in piece_stats if 'set_piece_priority_time' in p \
                             and p['fs_req'] > start_time \
                             and p['fs_req'] < piece_stat['fs_req'] \
                             and 'rank' in p \
                             and p['rank'] < piece_stat['rank']]) \
                             + len([p for p in piece_stats if p['profile_outcome'] == 'false_positive' \
                                        and 'rank' in p \
                                        and p['rank'] < piece_stat['rank']])
                piece_stat['rank_deviation'] = piece_stat['rank'] - access_rank
            else:
                access_rank = -1
            

            f.write("%d\t%-20s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%4d %4d %4d\n" % \
                        (piece_stat['piece'], piece_stat['profile_outcome'], piece_stat['attributed_delay'], piece_stat['early'] ,piece_stat['fs_req'] - start_time,\
                             set_piece_priority_offset, incoming_piece_offset,\
                             1 + float(counter) / 42 ,\
                             piece_stat['min_access_time'], piece_stat['mean_access_time'], piece_stat['frequency'], piece_stat['rank'], access_rank, piece_stat['rank'] - access_rank))
       
            if piece_stat['fs_req'] > start_time:
                access_counter += 1

        except KeyError, e:
#            f.write("catch\t%s\n" % str(e))
            f.write("%d\t%-20s\t%f\t%f\t%f\t%f\t%f\t%f\n" % \
                        (piece_stat['piece'], piece_stat['profile_outcome'], piece_stat['attributed_delay'], piece_stat['early'] ,piece_stat['fs_req'] - start_time,\
                             set_piece_priority_offset, incoming_piece_offset,\
                             1 + float(counter) / 42))

        counter += 1

    h=open("network.delay.distribution.tab",'w')
    f.write(''.join(["%s\t%-20s\t%f\n" % \
                         ('TOTAL', outcome, sum([p['attributed_delay'] for p in piece_stats if p['profile_outcome'] == outcome and 'attributed_delay' in p ])) \
                         for outcome in [ 'true_positive', 'true_positive_miss', 'false_negative', 'false_positive']]))

    f.write("%s\t%-20s\t%f\n" % \
                ('TOTAL', 'ALL', sum([p['attributed_delay'] for p in piece_stats if 'attributed_delay' in p ])))

    h.write(''.join(["%-20s\t%f\t%d\n" % \
                         (outcome, sum([p['attributed_delay'] for p in piece_stats if p['profile_outcome'] == outcome and 'attributed_delay' in p ]), \
                              len([p['attributed_delay'] for p in piece_stats if p['profile_outcome'] == outcome and 'attributed_delay' in p ])) \
                         for outcome in [ 'true_positive', 'true_positive_miss', 'false_negative', 'false_positive']]))

    h.write("%-20s\t%f\t%d\n" % \
                ('all', sum([p['attributed_delay'] for p in piece_stats if 'attributed_delay' in p ]), len([p['attributed_delay'] for p in piece_stats if 'attributed_delay' in p ])))
 
    f.close()
    g.close()
    h.close()    


    h=open("network.delay.rank.tab",'w')
    h.write("RANK_DEV\tMISSES\tHITS\tCUM_MISSES\tCUM_HITS\COND_PROB_MISS\n")
    data = sorted([p for p in piece_stats if 'rank_deviation' in p], key=itemgetter('rank_deviation'))
    #print data
    c_misses=0
    c_hits=0
    for k, grp in groupby(data, itemgetter('rank_deviation')): 
        lgrp=[g for g in grp]
        misses=len([g for g in lgrp if g['profile_outcome'] == 'true_positive_miss'])
        hits=len([g for g in lgrp if g['profile_outcome'] == 'true_positive'])
        c_misses += misses
        c_hits += hits
        h.write("%d\t\t%d\t%d\t%d\t\t%d\f\n" % (k, misses, hits, c_misses, c_hits, float(misses)/(misses+hits)))
    h.close()    


## END ANALYSIS


### PROCESS INPUT

exec_trial_log = False
fs_log         = False
p2pm_log       = False
libtorrent_log = False

print "parsing files"
try:
    parse_log('exec_trial.log')
    exec_trial_log = True
except IOError:
    print "no exec_trial.log"
try: 
    parse_log('p2pm.log')
    p2pm_log = True
except IOError: 
    print "no p2pm.log"
try: 
    parse_log('fs.log')
    fs_log = True
except IOError: 
    print "no fs.log"
try: 
    parse_log('libtorrent.log')
    libtorrent_log = True
except IOError:
    print "no libtorrent.log"

if not exec_trial_log:
    start_time=sorted(time_event.keys())[0]

f=open("run_stats.tab",'w')
f.write("start_time\t%f\n" % (start_time))
f.write("last_profile_request\t%f\n" % (last_profile_request))
if last_profile_request > 0:
    last_profile_offset = last_profile_request - start_time
else:
    last_profile_offset = 0

f.write("last_profile_offset\t%f\n" % (last_profile_offset))
f.close()

if fs_log:
    print "fs stats"
    build_fs_stats()

if p2pm_log and libtorrent_log:
    print "piece stats"
    build_piece_stats()

exit(0)
