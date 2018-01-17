import numpy as np
from datetime import datetime
import matplotlib
matplotlib.use('Agg') # Must be before importing matplotlib.pyplot or pylab!
import matplotlib.pyplot as plt

RSYNC_OUT = "rsync_logtime.txt"
IPFS_OUT = "ipfs_logtime.txt"

def rsync_file(rsync_results):
	with open(RSYNC_OUT, "r") as fd:
		lines_skip = 3
		for line in fd:
			if lines_skip > 0:
				lines_skip -= 1
				continue
			time = line.strip('\n').split(':')[1]
			rsync_results.append(time.split()[0])
			lines_skip = 2

def ipfs_file(ipfs_results):
	return

def main(num_trials, num_nodes):
	rsync_results = []
	# 4.52781414986
	# 4.44417500496
	# 4.3989739418
	rsync_file(rsync_results)
	ipfs_results = [4.357, 3.070, 3.163]
	ipfs_file(ipfs_results)

	## DELETE THE LINE BELOW
	# ipfs_results = rsync_results # DELETE LINE

	plotgraphs(num_nodes, rsync_results, ipfs_results)

def plotgraphs(num_nodes, rsync_results, ipfs_results):
	ind = np.arange(num_nodes)  # the x locations for the groups
	width = 0.15       # the width of the bars
	r_res = np.array(rsync_results)
	i_res = np.array(ipfs_results)
	print r_res
	fig, ax = plt.subplots()
	rects1 = ax.bar(ind, r_res, width, color='r')

	rects2 = ax.bar(ind+width, i_res, width, color='b')

	# add some text for labels, title and axes ticks
	ax.set_ylabel('Seconds')
	ax.set_ylim([0,6])
	ax.set_title('Download time per node')
	ax.set_xticks(ind + 2*width / 2)
	ticks = []
	for i in range(0, num_nodes):
		ticks.append('N'+str(i+1))
	ax.set_xticklabels(ticks)

	#autolabel(ax, rects1)
	#autolabel(ax, rects2)
	#autolabel(ax, rects3)

	# ax.legend((rects1[0], rects2[0]), ('Test P2P', 'SCP Tar'))
	# ax.legend(rects1, 'rsync')
	ax.legend((rects1[0], rects2[0]), ('rsync', 'VMTorrent (ipfs)'))

	plt.savefig('time_trial.png')

if __name__ == "__main__":
   num_trials = 1
   num_nodes = 3
   main(num_trials, num_nodes)