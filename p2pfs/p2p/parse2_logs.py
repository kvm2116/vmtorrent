import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt

SRC_IP = "10.138.0.2"

def load_peers(filename):
    f = open(filename, 'r')
    return f.read().split(' ')

def parse_log(num_trials, num_nodes):
    all_p = []
    peers = load_peers('dest_ips2.txt')
    received_trial_src = []
    received_trial_peers = []
    for t in range(2, num_trials+2):
        print "TRIAL "+str(t-1)+"*****************"
        t_pieces = {}
        sent_src_pieces = []
        sent_peer_pieces = []
        received_src_pieces = []
        received_peer_pieces = []
        finished_pieces = []
        for i in range(num_nodes):
            sent_src_pieces.append({})
            sent_peer_pieces.append({})
            received_peer_pieces.append({})
            received_src_pieces.append({})
        # read nodes
        for n in range(1, num_nodes+1):
            filename = "logp2p_"+str(t)+"-"+str(n)+".txt"
            print "NODE "+str(n)+"***************"
            print filename
            read_log(filename, t_pieces, sent_src_pieces, sent_peer_pieces, received_src_pieces, received_peer_pieces, n-1, peers)
            print "\n"
            mainsession = "main_session_"+str(t)+"-"+str(n)+".txt"
            read_main_session(mainsession, finished_pieces)
        # read src
        print "SRC *********************"
        filename = "p2p_logs_"+str(t)+".txt"
        read_src_log(filename, t_pieces, received_src_pieces,  peers)
        print "SENT SRC vs Peer pieces"
        print "SRC"
        print sent_src_pieces
        print "Peer"
        print sent_peer_pieces
        print "RECEIVED SRC vs Peer pieces"
        print "SRC"
        print received_src_pieces
        print "Peer"
        print received_peer_pieces
        received_trial_src.append(received_src_pieces)
        received_trial_peers.append(received_peer_pieces)
        print "\n"
        all_p.append(t_pieces)
        make_piece_plot_trial(t, num_nodes, received_src_pieces, received_peer_pieces)

        make_piece_plot_trial_percentage(t, num_nodes, received_src_pieces, received_peer_pieces)
        print "FINISHED PIECES"
        print "FINISHED PIECES"
        print finished_pieces

    for i in range(0, num_trials):
        print "TOTAL PIECES FOR TRIAL "+str(i+1)
        print all_p[i]

def read_main_session(filename, finished_pieces):
    f = open(filename, 'r')
    pieces = 0
    for line in f:
        if "PIECE_FINISHED" in line:
            pieces += 1
    f.close()
    finished_pieces.append(pieces)
    #make_piece_plot(trial_src, trial_peers, num_trials, num_nodes)

def make_piece_plot_trial_percentage(trial_no, num_nodes, received_src_pieces, received_peer_pieces):
    percentage = []
    for i in range(num_nodes):
        percentage.append(100*(float(len(received_src_pieces[i].keys()))/float((len(received_src_pieces[i].keys())+len(received_peer_pieces[i].keys())))))
    ind = np.arange(num_nodes)
    width = 0.2

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, percentage, width, color='r')

    ax.set_title("Percentage of Pieces Received from Source Per Node")
    ax.set_ylabel("Percentage")
    ax.set_xticks(ind + 2*width/2)
    ticks = []
    for i in range(0, num_nodes):
        ticks.append('N'+str(i+1))
    ax.set_xticklabels(ticks) 
    for tick in ax.xaxis.get_major_ticks():
        tick.label.set_fontsize(10)
    
    plt.savefig('piece_nodes_percentage'+str(trial_no)+'.png')

def make_piece_plot_trial(trial_no, num_nodes, received_src_pieces, received_peer_pieces):

    src_pieces = []
    peer_pieces = []
    for i in range(num_nodes):
        src_pieces.append(len(received_src_pieces[i].keys()))
        peer_pieces.append(len(received_peer_pieces[i].keys()))
    
    ind = np.arange(num_nodes)  # the x locations for the groups
    width = 0.1       # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, src_pieces, width, color='r')
    
    rects2 = ax.bar(ind+width, peer_pieces, width, color='b')
    

    #rects3 = ax.bar(ind+2*width, total_results['avg_nodes'][2], width, color='g')
    
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Number of Pieces')
    ax.set_title('Number of Pieces Received By Peers or Source Per Node')
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

    ax.legend((rects1[0], rects2[0]), ('Source', 'Peers'), loc=2)

    plt.savefig('piece_nodes'+str(trial_no)+'.png')


def make_piece_plot(trial_src, trial_peers, num_trials, num_nodes):
    print trial_src
    print trial_peers
    cumu_src = [0]*num_nodes
    cumu_peers = [0]*num_nodes
    for n in range(0, num_trials):
        for i in range(0, num_nodes):
            cumu_src[i] += trial_src[n][i]
            cumu_peers[i] += trial_peers[n][i]
    #avg_src = avg_src * (1/num_nodes)
    #avg_peers = avg_peers * (1/num_nodes)
    avg_src = [i / num_nodes for i in cumu_src] 
    avg_peers = [i / num_nodes for i in cumu_peers]
    print avg_src
    print avg_peers
    
    ind = np.arange(num_nodes)  # the x locations for the groups
    width = 0.1       # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, avg_src, width, color='r')
    
    rects2 = ax.bar(ind+width, avg_peers, width, color='b')
    

    #rects3 = ax.bar(ind+2*width, total_results['avg_nodes'][2], width, color='g')
    
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Number of Piece Messages')
    ax.set_title('Number of Piece Messages Per Node Over 5 Trials')
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

    ax.legend((rects1[0], rects2[0]), ('SRC', 'Peers'), loc=2)

    plt.savefig('piece_nodes.png')


def read_log(filename, t_pieces, sent_src_pieces, sent_peer_pieces, received_src_pieces, received_peer_pieces, node, peers):
    log = open(filename, 'r')
    connection = ""
    pieces = {}
    sent_pieces = {}
    received_pieces = {}
    ip = ""
    for line in log:
        if ('INCOMING' in line or 'OUTGOING' in line) and '[' in line:
            if connection != line:
                # new connection
                if len(sent_pieces)>0 or len(received_pieces)>0:
                    print connection.rstrip('\n')
                if len(sent_pieces)>0:
                    print "sent_pieces"
                    print sent_pieces
                if len(received_pieces)>0:
                    print "received_pieces"
                    print received_pieces
                sent_pieces = {}
                received_pieces = {}
                connection = line
                ip = line.split('[ ')[1].split(' ')[1]
                if ip not in pieces:
                    pieces[ip] = {}
        if '==> PIECE' in line:
            piece_no = "piece_"+line.split("[ ")[1].split(' ')[1]
            if piece_no not in sent_pieces:
                sent_pieces[piece_no] = 1
            else:
                sent_pieces[piece_no] += 1
            if 'sent_pieces' not in pieces[ip]:
                pieces[ip]['sent_pieces']={}
            if piece_no not in pieces[ip]['sent_pieces']:
                pieces[ip]['sent_pieces'][piece_no] = 1
            else:
                pieces[ip]['sent_pieces'][piece_no] +=1
            if piece_no not in t_pieces:
                t_pieces[piece_no] = 1
            else:
                t_pieces[piece_no] += 1
        elif '<== PIECE' in line:
            piece_no = "piece_"+line.split("[ ")[1].split(' ')[1]
            if piece_no not in received_pieces:
                received_pieces[piece_no] = 1
            else:
                received_pieces[piece_no] += 1

            if 'received_pieces' not in pieces[ip]:
                pieces[ip]['received_pieces'] = {}
            if piece_no not in pieces[ip]['received_pieces']:
                pieces[ip]['received_pieces'][piece_no] = 1
            else:
                pieces[ip]['received_pieces'][piece_no] += 1
            if piece_no not in t_pieces:
                t_pieces[piece_no] = 1
            else:
                t_pieces[piece_no] += 1
    print "TOTAL PIECES FOR NODE"
    print pieces

    cumulative_ips_sent = {}
    cumulative_ips_received = {}
    for ip in pieces:
        total_pieces = 0
        if 'sent_pieces' in pieces[ip]:
            cumulative_ips_sent[ip] = 0
            for p in pieces[ip]['sent_pieces']:
                cumulative_ips_sent[ip] += pieces[ip]['sent_pieces'][p]
                total_pieces += pieces[ip]['sent_pieces'][p]
                if ip.startswith(SRC_IP):
                    if p not in sent_src_pieces[node]:
                        sent_src_pieces[node][p] = 1
                    else:
                        sent_src_pieces[node][p] += 1
                else:
                    if p not in sent_peer_pieces[node]:
                        sent_peer_pieces[node][p] = 1
                    else:
                        sent_peer_pieces[node][p] += 1
                for i in range(len(peers)):
                    if ip.startswith(peers[i]):
                        if p not in received_peer_pieces[i]:
                            received_peer_pieces[i][p] = 1
                        else:
                            received_peer_pieces[i][p] += 1
        if 'received_pieces' in pieces[ip]:
            cumulative_ips_received[ip] = 0
            for p in pieces[ip]['received_pieces']:
                cumulative_ips_received[ip] += pieces[ip]['received_pieces'][p]
                total_pieces += pieces[ip]['received_pieces'][p]
                if ip.startswith(SRC_IP):
                    if p not in received_src_pieces[node]:
                        received_src_pieces[node][p] = 1
                    else:
                        received_src_pieces[node][p] += 1
                else:
                    if p not in received_peer_pieces[node]:
                        received_peer_pieces[node][p] = 1
                    else:
                        received_peer_pieces[node][p] += 1


    print "DESTINATIONS ORDERED FOR NODE"
    print "BY NUMBER OF SENT PIECES"
    print sorted(cumulative_ips_sent.items(), key=lambda x: x[1])
    print "BY NUMBER OF RECEIVED PIECES"
    print sorted(cumulative_ips_received.items(), key=lambda x: x[1])
    log.close()

def read_src_log(filename, t_pieces, received_src_peers, peers):
    log = open(filename, 'r')
    connection = ""
    pieces = {}
    sent_pieces = {}
    received_pieces = {}
    ip = ""
    for line in log:
        if ('INCOMING' in line or 'OUTGOING' in line) and '[' in line:
            if connection != line:
                # new connection
                if len(sent_pieces)>0 or len(received_pieces)>0:
                    print connection.rstrip('\n')
                if len(sent_pieces)>0:
                    print "sent_pieces"
                    print sent_pieces
                if len(received_pieces)>0:
                    print "received_pieces"
                    print received_pieces
                sent_pieces = {}
                received_pieces = {}
                connection = line
                ip = line.split('[ ')[1].split(' ')[1]
                if ip not in pieces:
                    pieces[ip] = {}
        if '==> PIECE' in line:
            piece_no = "piece_"+line.split("[ ")[1].split(' ')[1]
            if piece_no not in sent_pieces:
                sent_pieces[piece_no] = 1
            else:
                sent_pieces[piece_no] += 1
            if 'sent_pieces' not in pieces[ip]:
                pieces[ip]['sent_pieces']={}
            if piece_no not in pieces[ip]['sent_pieces']:
                pieces[ip]['sent_pieces'][piece_no] = 1
            else:
                pieces[ip]['sent_pieces'][piece_no] +=1
            if piece_no not in t_pieces:
                t_pieces[piece_no] = 1
            else:
                t_pieces[piece_no] += 1
        elif '<== PIECE' in line:
            piece_no = "piece_"+line.split("[ ")[1].split(' ')[1]
            if piece_no not in received_pieces:
                received_pieces[piece_no] = 1
            else:
                received_pieces[piece_no] += 1

            if 'received_pieces' not in pieces[ip]:
                pieces[ip]['received_pieces'] = {}
            if piece_no not in pieces[ip]['received_pieces']:
                pieces[ip]['received_pieces'][piece_no] = 1
            else:
                pieces[ip]['received_pieces'][piece_no] += 1
            if piece_no not in t_pieces:
                t_pieces[piece_no] = 1
            else:
                t_pieces[piece_no] += 1

    print "TOTAL PIECES FOR NODE"
    print pieces

    cumulative_ips_sent = {}
    cumulative_ips_received = {}
    for ip in pieces:
        if 'sent_pieces' in pieces[ip]:
            cumulative_ips_sent[ip] = 0
            for p in pieces[ip]['sent_pieces']:
                cumulative_ips_sent[ip] += pieces[ip]['sent_pieces'][p]
        if 'received_pieces' in pieces[ip]:
            cumulative_ips_received[ip] = 0
            for p in pieces[ip]['received_pieces']:
                cumulative_ips_received[ip] += pieces[ip]['received_pieces'][p]
        for i in range(0, len(peers)):
            if ip.startswith(peers[i]):
                for p in pieces[ip]['sent_pieces']:
                    if p not in received_src_peers[i]:
                        received_src_peers[i][p] = 1
                    else:
                        received_src_peers[i][p] += 1

    print "DESTINATIONS ORDERED FOR NODE"
    print "BY NUMBER OF SENT PIECES"
    print sorted(cumulative_ips_sent.items(), key=lambda x: x[1])
    print "BY NUMBER OF RECEIVED PIECES"
    print sorted(cumulative_ips_received.items(), key=lambda x: x[1])
    log.close()


def main():
    parse_log(5, 20)

if __name__ == "__main__":
    main()
