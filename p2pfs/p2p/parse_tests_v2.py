import numpy as np
from datetime import datetime
import matplotlib
matplotlib.use('Agg') # Must be before importing matplotlib.pyplot or pylab!
import matplotlib.pyplot as plt

def test_p2p_parse(num_trials, num_nodes, total_results):

    total_results['avg_nodes'].append([0]*num_nodes)

    src = open('logtime.txt', 'r+')    
    results = []
    trial = 1
    for line in src:
        if line.startswith('TRIAL'):
            new_trial = int(line.split(' ')[1])
            if trial != new_trial:
                trial = new_trial
            results.append({})
        if line.startswith('compress start'):
            results[trial-1]['c_start'] = datetime.strptime(line.rstrip('\n').split('start: ')[1], "%Y-%m-%d %H:%M:%S.%f")
        if line.startswith('compress end'):
            results[trial-1]['c_end'] = datetime.strptime(line.rstrip('\n').split('end: ')[1], "%Y-%m-%d %H:%M:%S.%f")
        if line.startswith('make torrent start'):
            results[trial-1]['mt_start'] = datetime.strptime(line.rstrip('\n').split('start: ')[1], "%Y-%m-%d %H:%M:%S.%f")
        if line.startswith('test p2p start'):
            results[trial-1]['tp2p_start'] = datetime.strptime(line.rstrip('\n').split('start: ')[1][:24], "%Y-%m-%d %H:%M:%S.%f")
#    print results

    for i in range(1, num_nodes+1):
        filename = 'log_testp2p_'+str(i)+'.txt'
        dst = open(filename, 'r+')
        trial = 1
        for line in dst:
            if line.startswith('TRIAL'):
                new_trial = int(line.split(' ')[1])
                if trial != new_trial:
                    trial = new_trial
            if line.startswith('test p2p start'):
                if 'dst_p2p_start' not in results[trial-1]:
                    results[trial-1]['dst_p2p_start'] = []
                results[trial-1]['dst_p2p_start'].append(datetime.strptime(line.rstrip('\n').split('start: ')[1], "%Y-%m-%d %H:%M:%S.%f"))
            if line.startswith('tar file received'):
                if 'tar_file_recv' not in results[trial-1]:
                    results[trial-1]['tar_file_recv'] = []
                results[trial-1]['tar_file_recv'].append(datetime.strptime(line.rstrip('\n').split('received: ')[1], "%Y-%m-%d %H:%M:%S.%f"))
            if line.startswith('decompress start'):
                if 'dc_start' not in results[trial-1]:
                    results[trial-1]['dc_start'] = []
                results[trial-1]['dc_start'].append(datetime.strptime(line.rstrip('\n').split('start: ')[1], "%Y-%m-%d %H:%M:%S.%f"))
            if line.startswith('decompress end'):
                if 'dc_end' not in results[trial-1]:
                    results[trial-1]['dc_end'] = []
                results[trial-1]['dc_end'].append(datetime.strptime(line.rstrip('\n').split('end: ')[1], "%Y-%m-%d %H:%M:%S.%f"))
           
    #print results

    compression_times = []
    decompression_times = []
    test_p2p_download = []
    total_test_p2p = []
 
    for trial in results:
        print "trial"
        #print trial
        trial['c_difference'] = (trial['c_end'] - trial['c_start']).total_seconds()
        print 'compression time: ' + str(trial['c_difference'])
        compression_times.append(trial['c_difference'])

        trial['d_difference'] = []
        trial['test_p2p_diff'] = []
        print trial
        trial['total_download'] = (max(trial['tar_file_recv']) - trial['tp2p_start']).total_seconds()

        for node in range(0, num_nodes):
            diff_dst = float((trial['tar_file_recv'][node] - trial['dst_p2p_start'][node]).total_seconds())
            diff_src = float((trial['tar_file_recv'][node] - trial['tp2p_start']).total_seconds())
            diff = diff_dst
            if diff_src < diff_dst:
                diff = diff_src
            trial['test_p2p_diff'].append(diff)
            print "test p2p diff "+str(node)+":"+str(diff)
            trial['d_difference'].append(float((trial['dc_end'][node] - trial['dc_start'][node]).total_seconds()))
            print "decompression time "+str(node)+":"+str(trial['d_difference'][node])

        trial['d_avg'] = np.mean(trial['d_difference'])
        decompression_times.append(trial['d_avg'])
        print "trial decompression time average: " + str(trial['d_avg'])
        print "trial decompression time stdev: " + str(np.std(trial['d_difference']))
        trial['test_p2p_avg'] = np.mean(trial['test_p2p_diff'])
        test_p2p_download.append(trial['test_p2p_avg'])
        print "trial test p2p average: " + str(trial['test_p2p_avg'])
        print "trial test p2p stdev: " + str(np.std(trial['test_p2p_diff']))
        print "download time for all nodes: " + str(trial['total_download'])
        total_test_p2p.append(trial['total_download'])

    total_results['avg_download_time'].append(np.mean(total_test_p2p))
    total_results['std_download_time'].append(np.std(total_test_p2p))
    total_results['avg_trials'].append(test_p2p_download)

    for node in range(0, num_nodes):
        times = []
        for trial in range(0, num_trials):
            times.append(results[trial]['test_p2p_diff'][node])
        total_results['avg_nodes'][0][node] = np.mean(times)
        
    print "TOTALS"
    print "average compression time: " + str(np.mean(compression_times))
    print "std compression time: " + str(np.std(compression_times))
    print "average decompression time: " + str(np.mean(decompression_times))
    print "std decompression time: " + str(np.std(decompression_times))
    print "average test_p2p download time: "+str(np.mean(test_p2p_download))
    print "std test_p2p download time: " + str(np.std(test_p2p_download))
    print "average total download time: " + str(np.mean(total_test_p2p))
    print "std total download time: " + str(np.std(total_test_p2p))
    

def scp_file(mode, num_trials, num_nodes, total_results, index):
    p2p_time = 2
    num_success = 0
    total_results['avg_nodes'].append([0]*num_nodes)

    if mode == 'tar':
        src_name = 'scp_tar_logtime.txt'
    else:
        src_name = 'scp_logtime.txt'
    src = open(src_name, 'r+')
    
    sent_times = []
    trial = 1
    for line in src:
        if line.startswith('TRIAL'):
            new_trial = int(line.split(' ')[1])
            if trial != new_trial:
                trial = new_trial
            node_list = [0]*num_nodes
            sent_times.append(node_list)
        if line.startswith('sent to machine'):
            words = line.split(' ')
            node = int(words[3])-1
            sent_times[trial-1][node] = datetime.strptime(line.rstrip('\n').split('at: ')[1], "%Y-%m-%d %H:%M:%S.%f")
 #   print sent_times

    received_times = []
    for i in range(1, num_nodes+1):
        if mode == 'tar':
            filename = 'scp_tar_logfile_'+str(i)+'.txt'
        else:
            filename = 'scp_logfile_'+str(i)+'.txt'
        dst = open(filename, 'r+')
        trial = 1
        for line in dst:
            if line.startswith('TRIAL'):
                new_trial = int(line.split(' ')[1])
                if trial != new_trial:
                    trial = new_trial
                node_list = [0]*num_nodes
                if(len(received_times)<num_trials):
                    received_times.append(node_list)
            if line.startswith('entire folder received'):
                node = i - 1
                received_times[trial-1][node] = datetime.strptime(line.rstrip('\n').split('received: ')[1], "%Y-%m-%d %H:%M:%S.%f")
                if (received_times[trial-1][node]-sent_times[trial-1][0]).total_seconds() < p2p_time:
                    num_success +=1
  #  print received_times

    avg_node_recv = []
    total_time = []
    node_recv = []
    for t in range (0, num_trials):
        node_recv.append([0]*num_nodes)
        print "trial"
        for n in range (0, num_nodes):
            node_recv[t][n] =(received_times[t][n] - sent_times[t][n]).total_seconds()
            print "scp node download time: "+str(node_recv[t][n])
        avg_node_recv.append(np.mean(node_recv[t]))
        print "trial average scp node download time: "+str(avg_node_recv[t])
        print "trail std scp node download time: "+str(np.std(node_recv))
        total_diff = (max(received_times[t])-min(sent_times[t])).total_seconds()
        print "trial total download time: "+str(total_diff)
        total_time.append(total_diff)
   
    
    total_results['avg_trials'].append(avg_node_recv)
    total_results['avg_download_time'].append(np.mean(total_time))
    total_results['std_download_time'].append(np.std(total_time))
    for node in range(0, num_nodes):
        times = []
        for trial in range(0, num_trials):
            times.append(node_recv[trial][node])
        total_results['avg_nodes'][index][node] = np.mean(times)
 
    print "TOTALS"
    print "average scp node download time: "+str(np.mean(avg_node_recv))
    print "average total download time: "+str(np.mean(total_time))
    print "std total download time: "+str(np.std(total_time))
    print "Nodes downloaded successfully: "+str(num_success/num_trials)

def autolabel(ax, rects):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., 1.05*height,
                '%f' % height,
                ha='center', va='bottom')

def main(num_trials, num_nodes):
    total_results = {}
    total_results['avg_download_time'] = []
    total_results['std_download_time'] = []
    total_results['avg_trials'] = []
    total_results['avg_nodes'] = []
    # print "TEST P2P TRIALS"
    # test_p2p_parse(num_trials, num_nodes, total_results)
    #print "SCP RECURSIVE TRIALS"
    #scp_file('recursive', num_trials, num_nodes, total_results)
    # print "SCP TAR TRIALS"
    # scp_file('tar', num_trials, num_nodes, total_results)

    print "SCP TRIALS"
    index = 0
    scp_file('', num_trials, num_nodes, total_results, index)

    print total_results

    avg_time_trial_graph(total_results, num_trials)
    # avg_time_graph(total_results)
    avg_time_node_graph(total_results, num_nodes)

def avg_time_graph(total_results):
    ind = np.arange(2)  # the x locations for the groups
    width = 0.45       # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, total_results['avg_download_time'], width, color='r', yerr=total_results['std_download_time'])
    
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Seconds')
    ax.set_title('Total download times for 20 nodes')
    ax.set_xticks(ind + width / 2)
    # ax.set_xticklabels(('Test_P2P', 'SCP TAR'))
    ax.set_xticklabels(('SCP TAR'))
    ax.set_ylim([0, max(total_results['avg_download_time'])+20])
    autolabel(ax, rects1)

    plt.savefig('avg_download_times.png')

def avg_time_trial_graph(total_results, num_trials):
    ind = np.arange(num_trials)  # the x locations for the groups
    width = 0.15       # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, total_results['avg_trials'][0], width, color='r')
    
    # rects2 = ax.bar(ind+width, total_results['avg_trials'][1], width, color='b')
    

    #rects3 = ax.bar(ind+2*width, total_results['avg_trials'][2], width, color='g')
    
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Seconds')
    ax.set_title('Average download time per trial')
    ax.set_xticks(ind + 2*width / 2)
    ax.set_xticklabels(('T1', 'T2', 'T3', 'T4', 'T5'))

    #autolabel(ax, rects1)
    #autolabel(ax, rects2)
    #autolabel(ax, rects3)

    # ax.legend((rects1[0], rects2[0]), ('Test P2P', 'SCP Tar'))
    ax.legend((rects1), ('SCP Tar'))
    
    plt.savefig('avg_time_trials.png')

def avg_time_node_graph(total_results, num_nodes):
    ind = np.arange(num_nodes)  # the x locations for the groups
    width = 0.1       # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, total_results['avg_nodes'][0], width, color='r')
    
    # rects2 = ax.bar(ind+width, total_results['avg_nodes'][1], width, color='b')
    

    #rects3 = ax.bar(ind+2*width, total_results['avg_nodes'][2], width, color='g')
    
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Seconds')
    ax.set_title('Average download time per node')
    ax.set_xticks(ind + 2*width / 2)
    ticks = []
    for i in range(0, num_nodes):
	ticks.append('N'+str(i+1))
    ax.set_xticklabels(ticks)

    for tick in ax.xaxis.get_major_ticks():
        tick.label.set_fontsize(10) 
    #autolabel(ax, rects1)
    #autolabel(ax, rects2)
    #autolabel(ax, rects3)

    # ax.legend((rects1[0], rects2[0]), ('Test P2P', 'SCP Tar'), loc=2)
    ax.legend((rects1), ('SCP Tar'), loc=2)

    plt.savefig('avg_time_node.png')



if __name__ == "__main__":
   num_trials = 1
   num_nodes = 1
   main(num_trials, num_nodes)

            
