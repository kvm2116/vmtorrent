#! /usr/bin/env python

import sys
import corestats
import re

if len(sys.argv) != 3:
    print 'Usage: parse_trials.py summary #nodes'
    print '- parses the exec_trials.log'
    exit();


name=sys.argv[1]
num_nodes=int(sys.argv[2])
print "%s" % (name)
vmtimes = {}
net_delay = {}
def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False

begin_summary = False
infile = open(name,'r')
for line in infile:

##  UNCOMMENT IF PARSING exec_trials.log
#    if not begin_summary:
#        m = re.search('Summary of trial results', line) 
#        if m is None:
#            continue
#        else:
#            begin_summary = True
        
    cols = line.split()
    if len(cols) >= 4 and is_number(cols[3]):
        try:
            vmtimes[cols[1]][int(cols[2])-1].append(float(cols[3]))
        except KeyError:
            vmtimes[cols[1]] = [[float(cols[3])]]
        except IndexError:
            vmtimes[cols[1]].append([float(cols[3])])
        if len(cols) == 5 and is_number(cols[4]):
            try: 
                net_delay[cols[1]][int(cols[2])-1].append(float(cols[4]))
            except KeyError:
                net_delay[cols[1]] = [[float(cols[4])]]
            except IndexError:
                net_delay[cols[1]].append([float(cols[4])])
        else:
            try: 
                net_delay[cols[1]][int(cols[2])-1].append(float(0))
            except KeyError:
                net_delay[cols[1]] = [[float(0)]]
            except IndexError:
                net_delay[cols[1]].append([float(0)])
      


print "type\ttrial\t#nodes\t%10s  %10s  %10s  %10s\t%10s  %10s  %10s  %10s" % ( 'avg', 'std', 'min', 'max', 'd-avg', 'd-std', 'd-min', 'd-max' )
for (trial_id,trial_set) in vmtimes.items():
    counter=0 
    good_trial_set = []
    good_net_delay_set = []
    for trial in trial_set:
        if len(trial) == 1:
            print "%s\t%d\t%d\t%10.2f, %10.2f, %10.2f, %10.2f" % \
                (trial_id, counter, len(trial), \
                     trial[0], 0, trial[0], trial[0])
        else:
            stats = corestats.Stats(trial)
            try:
                d_stats = corestats.Stats(net_delay[trial_id][counter])
                print "%s\t%d\t%d\t%10.2f, %10.2f, %10.2f, %10.2f\t%10.2f, %10.2f, %10.2f, %10.2f" % \
                    (trial_id, counter, len(trial), \
                         stats.avg(), stats.stdev(), stats.min(), stats.max(), \
                         d_stats.avg(), d_stats.stdev(), d_stats.min(), d_stats.max())
            except:
                print "%s\t%d\t%d\t%10.2f, %10.2f, %10.2f, %10.2f" % \
                    (trial_id, counter, len(trial), \
                         stats.avg(), stats.stdev(), stats.min(), stats.max())

        if len(trial) >= 0.75*num_nodes:
#        if len(trial) == num_nodes:
            good_trial_set.extend(trial)
            good_net_delay_set.extend(net_delay[trial_id][counter])
        counter += 1
    if len(good_trial_set) > 1:
        stats = corestats.Stats(good_trial_set)
        d_stats = corestats.Stats(good_net_delay_set)
        print "%s\t%s\t%d\t%10.2f, %10.2f, %10.2f, %10.2f\t%10.2f, %10.2f, %10.2f, %10.2f" % \
            (trial_id, "ALL", len(good_trial_set)/num_nodes, \
                 stats.avg(), stats.stdev(), stats.min(), stats.max(),\
                 d_stats.avg(), d_stats.stdev(), d_stats.min(), d_stats.max())

