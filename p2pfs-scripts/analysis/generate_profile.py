#! /usr/bin/env python

import sys
import os
import re
from datetime import datetime, date, time
import string
import math
from optparse import OptionParser
from operator import itemgetter


# TO USE
def usage():
    print "usage: generate_profile.py profile_name src1 [src2 ... ]"
    print "e.g., - generate_profile.py profile_name `find directory_w_runs -name fs.block.accesses.tab | xargs echo -n`"
    exit(1)


# GLOBAL VARIABLES
BLOCK_SIZE=float(1)/64   # IN MB
NETWORK_RATE = 10.5      # MB / S
STARTUP_DELAY = 1.5      # S
CUTOFF=0.25               # BLOCKS THAT APPEAR W/ LOWER FREQUENCY THAN THIS NUMBER ARE LEFT OUT OF THE PROFILE

# PARSE INPUT
if len(sys.argv) < 3:
	usage()
	
profile_name = sys.argv[1]
filelist = sys.argv[2:]

filelist=map(lambda x: x.lstrip("\./"), filelist)



filelist_index = {}
i=0
for f in filelist:
	filelist_index[f] = i
	i += 1

if len(filelist)<1:
	usage()



# read all files 
# For each block, create a list of block rank and access times (taken from all files)

block_info = {}
block_appearances = {}
block_clusters = {}
ranked_blocks = []

for file_name in filelist:
        print "processing %s" % (file_name)
	infile = open(file_name,'r')
	rank=0
	for line in infile:
		if line.startswith("#"):
			continue
		cols = line.split()
                try:
                    block = int(cols[1])
                    time =  float(cols[0])
                except:
                    continue
		if not block_info.has_key(block):
			block_info[block] = [(rank,time)]
		else:
			block_info[block].append((rank,time))
		rank=rank+1;

		try:
			block_appearances[block].append(filelist_index[file_name])
		except:
			block_appearances[block] = [filelist_index[file_name]]

	infile.close()	


print "determining block clusters";
# DETERMINE THE BLOCK CLUSTERS (NAIVE METHOD - CLUSTERS COMPRISE ALL BLOCKS THAT SHOW UP IN THE EXACT SAME SET OF RUNS)			
for (block,appearances) in sorted(block_appearances.items(),key=itemgetter(0)):
	appearances_string = ''.join(["%d," % (appearance) for appearance in appearances])
	try:
		block_clusters[appearances_string].append(block)
	except: 
		block_clusters[appearances_string] = [block]


print "producing block cluster histograms";
# PRODUCE A HISTOGRAM OF BLOCK CLUSTER SIZES
cluster_size_histogram = {}
block_cluster_index = {}
i = 0
for (cluster,block_list) in sorted(block_clusters.items(), key=lambda (k,v): len(v), reverse=True):
	for block in block_list:
		block_cluster_index[block] = i
	i += 1
	try:
		cluster_size_histogram[len(block_list)] += 1
	except KeyError:
		cluster_size_histogram[len(block_list)] = 1
			
f=open(profile_name+'.profile.cluster_size.histogram','w')
for (cluster_size, pop) in sorted(cluster_size_histogram.items(), key=itemgetter(0), reverse=True):
	f.write("%d\t%d\n" % (cluster_size,pop))
f.close()

			
print "computing block profile statistics";
# COMPUTE BLOCK PROFILE STATISTICS
fraction_counter={}
for block,v in block_info.iteritems():
	if not v is None:
		order_stats=sorted(v,key=itemgetter(1))
		rank = sum(el[0] for el in v)/len(v);
		min_time = order_stats[int(0.1*len(v))][1]
		med_time = order_stats[len(v)/2][1]
		max_time = order_stats[int(0.9*len(v))][1]
		mean_time = sum(el[1] for el in v)/len(v);
		time_spread = max_time - min_time
		#stddev_time = 0
		#if len(v) > 1:
		#	stddev_time = sum([ (mean_time - el[1]) ** 2 for el in v]) / (len(v) - 1)
		fraction = float(len(v))/len(filelist);
		try:
			fraction_counter[fraction] += 1
		except KeyError:
			fraction_counter[fraction] = 1
		ranked_blocks.append([block,rank,mean_time,fraction,time_spread,min_time,max_time,med_time,block_cluster_index[block]]);
       		

print "creating profile";
# CREATE THE PROFILE BASED ON THE SORTING CRITERION
#criterion=7  # MED_TIME
criterion=2  # AVG_TIME
sorted_blocks = sorted(ranked_blocks, key=lambda x: x[criterion])
f=open(profile_name+'.profile','w')
f.write("#%s\t%s\t%s\t%s\t%s\t\t%s\t%s\t%s\t%s\n"% ('BLOCK','ELI_RANK','MEAN_TIME','FRACTION', 'SPREAD', 'MIN_TIME', 'MAX_TIME','MED_TIME','CLUSTER'))	
f.write(''.join(["%d\t%d\t\t%f\t%f\t%f\t%f\t%f\t%f\t%d\n" % (bl[0],bl[1],bl[2],bl[3],bl[4],bl[5],bl[6],bl[7],bl[8]) for bl in sorted_blocks if bl[3] >= CUTOFF ]))
f.close()


print "creating block frequence histogram";
# Create a histogram of the frequency w/ which blocks show up in a given run
f=open(profile_name+'.profile.histogram','w')
f.write("#FRACTION\tCOUNT\tPDF\tCDF\n")
CDF=0
for (frac,count) in sorted(fraction_counter.items(), key=itemgetter(0)):
	CDF += float(count)/len(sorted_blocks)
	f.write("%f\t%d\t%f\t%f\n" % (frac,count,float(count)/len(sorted_blocks), CDF))
f.close()


print "calculating alternate access comparison";
# Record the relative access/download progress of the profiled runs, vs. what would be expected from profile or demand
f=open(profile_name+'.profile.access.time','w')
f.write("#TIME\t\tBINDFS_READ\tPROFILE_DOWNLOADED\tPROFILE_LAG\tDEMAND_DOWNLOADED\tDEMAND_LAG\n")
f.write("%f\t%f\t%f\t%f\n" % (0,0,0,0))
bindfs_read=0
demand_downloaded=0
max_demand_lag=0
max_profile_lag=0
ltime=0
mb_to_time = { 'bindfs' : {}, 'demand' : {}  }

for bl in [ bl for bl in sorted_blocks ] :
	time = bl[criterion]
	dt = max(time - ltime, 0.000001)
	bindfs_read += bl[3]*BLOCK_SIZE  # THE FRACTION OF RUNS IN WHICH THIS BLOCK WAS NEEDED
	demand_downloaded = min(bindfs_read, demand_downloaded + NETWORK_RATE * dt)  	
	profile_downloaded = max(time - STARTUP_DELAY, 0) * NETWORK_RATE

	demand_lag = max((bindfs_read - demand_downloaded),0) / NETWORK_RATE 
	profile_lag = max((bindfs_read - profile_downloaded),0) / NETWORK_RATE 
	
	max_demand_lag = max(max_demand_lag, demand_lag)
	max_profile_lag = max(max_profile_lag, profile_lag)
		
	f.write("%f\t%f\t%f\t\t%f\t%f\t\t%f\n" % (time, bindfs_read, profile_downloaded, profile_lag ,demand_downloaded, demand_lag))
	ltime = time

f.close()


# Print out the lower bounds on delay based on assumptions used above (NETWORK RATE, STARTUP DELAY)
f=open(profile_name+'.profile.lower_bound','w')
f.write("#DEMAND\t\tPROFILE\n")
f.write("%f\t%f\n" % (max_demand_lag, max_profile_lag))
f.close()

		




