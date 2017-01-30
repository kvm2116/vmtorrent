#!/sw2/bin/python2.5

#!/Library/Frameworks/Python.framework/Versions/2.7/bin/python
# SWAP ORDER TO SWITCH BETWEEN X-WINDOWS AND 3-D CAPABLE

import sys
import re
import string
import copy
import math
from operator import itemgetter

import numpy as np

#doShow = True
doShow = False
doSave = True
#doSave = False
NETWORK_SPEED = 10 #MB
DEBUG=True
makeLegend=False
TEST=False

DL_STATS = [ { 'vm' : 'fedora2', 'tsk' : 'noop', 'DL' : 321 }, \
                 { 'vm' : 'fedora2', 'tsk' : 'latex', 'DL' : 356 }, \
                 { 'vm' : 'ubuntu2', 'tsk' : 'noop', 'DL' : 237 }, \
                 { 'vm' : 'ubuntu2', 'tsk' : 'latex', 'DL' : 253 }, \
                 { 'vm' : 'ubuntu2', 'tsk' : 'docEdit', 'DL' : 403 }, \
                 { 'vm' : 'win7', 'tsk' : 'noop', 'DL' : 296 }, \
                 { 'vm' : 'win7', 'tsk' : 'music', 'DL' : 349 }, \
                 { 'vm' : 'win7', 'tsk' : 'powerPoint', 'DL' : 342 } ]

def number_cast(s):
    try:
        s=int(s)
    except ValueError:
        try:
            s=float(s)
        except ValueError:
            pass

    return s


# THE GLOBAL DATABASE
db = []


def parse_file(arg):

    ignore = [ 'profile', 'exe', 'sh', 'exec_trials' ]

    # PARSE FILE NAME
    fields = arg.split('.')
    
    population = map(lambda x: int(x.lstrip('s').lstrip('n')),fields[0].split('-'))
    dkey = { '#servers' : population[0], '#peers' : population[1] }
    for field in fields[1:]:
        
        if field in ignore:
            continue
        
        keyval=field.split('_')
        
        if len(keyval) != 2:
            continue

        dkey[keyval[0]] = number_cast(keyval[1])

    # PARSE SUMMARY FILE CONTENT
    infile = open(arg,'r')
    field_names=[]
    for line in infile:

        # IGNORE NON-DATA LINES
        m = re.search('log', line)
        if not m is None:
            continue

        m = re.search('dat', line)
        if not m is None:
            continue

        # REMOVE ANY COMMAS AND SPLIT ON WHITESPACE
        line = re.sub(',', '', line)
        fields = line.split()
        fields = map(number_cast, fields)
        
        # GET SUMMARY FIELDS
        m = re.search('#', line)
        if not m is None:
            field_names.extend(fields)
            continue

        # CREATE A NEW ENTRY FOR THE DATABASE
        new_dkey = copy.copy(dkey)
        i=0
        for field in fields:
            try:
                new_dkey[field_names[i]] = field
            except:
                continue

            i += 1

        if len(fields) > 0:
            if not 'type' in new_dkey:
                print "problematic key %s" % (new_dkey)
                print "infile %s" % (arg)
                print "source line %s" % (line)
            else:
                db.append(new_dkey)
#                if DEBUG:
#                    print "new_dkey = %s" % (new_dkey)


def get_range(val_name, dict_list):
    return sorted(list(set([ i[val_name] for i in dict_list if val_name in i])))
        

def m_and(list):
    return (sum(list)/len(list) > 0)


def in_slice(i, cond_dict):
#    return m_and([k in i and i[k] == v for (k,v) in cond_dict.items()])
    return m_and([ i[k] == v for (k,v) in cond_dict.items() if k in i])
    
        
def plot_size_vs_runtime(conds):

    conds['trial'] = 'ALL'

    import matplotlib as mpl
    import matplotlib.pyplot as plt

#    mpl.rcParams['legend.fontsize'] = 'small'
#    mpl.rcParams['legend.markerscale'] = 0.5
    mpl.rcParams['legend.handlelen'] = 0.15     # 0.05   # the length of the legend lines
    mpl.rcParams['legend.handletextsep'] = 0.05
    mpl.rcParams['legend.labelsep'] = 0.07 #     : 0.010  # the vertical space between the legend entries    
    mpl.rcParams['xtick.labelsize'] = 16
    mpl.rcParams['ytick.labelsize'] = 16

    fig = plt.figure(figsize=(5,4))
    fig.subplots_adjust(left=0.147)
    fig.subplots_adjust(right=0.955)
    fig.subplots_adjust(top=0.975)
    fig.subplots_adjust(bottom=0.1285)
    ax = fig.gca()
    ax.set_autoscale_on(False)


    msize =      [   16,   16,    16,    18,     6,    16,   16]
    lwidth =     [    6,    3,     3,     3,     6,     5,    4]
    line_formats=['r:', 'r-^', 'y-s', 'g-o', 'g:o', 'b--', 'k-']
    msize.reverse()
    lwidth.reverse()
    line_formats.reverse()
    plots=[]
    plotnames=[]

    # CALCULATE THE LOWER BOUND ON RUNTIME BASED ON ACCESS SIZE AND NETWORK SPEEK
    required_access_size = [ i['DL'] for i in DL_STATS if i['vm'] == conds['vm'] and i['tsk'] == conds['tsk'] ][0]
    lb_runtime = required_access_size / NETWORK_SPEED
    print "lb_runtime = %f" % (lb_runtime)

    # GET THE ACTUAL CS TIME ON THE NETWORK
    conds['type'] = 'sshfs'
    conds['#peers'] = 1
    slice = [ i for i in db if in_slice(i,conds) ]
    if DEBUG:
        print "========== cs time slice   ================="
        print slice
        print "---------------------------------------------"
    try:
        cs_runtime = slice[0]['avg']
        print "cs_runtime = %f" % (cs_runtime)
        cs_rtt_component = cs_runtime - lb_runtime
        print "cs_rtt_component = %f" % (cs_rtt_component)
    except:
        pass
    del conds['#peers']

    # GET BINDFS NUMBER FOR LOCAL AND RAM DISK
    conds['type'] = 'bindfs'
   
    # GET RAM DISK NUMBERS
    conds['dsk'] = 'ram'
    ram_runtime = 1
    slice = [ i for i in db if in_slice(i,conds) ]
    if DEBUG:
        print "========== ram_disk slice   ================="
        print slice
        print "---------------------------------------------"
    try:
        ram_runtime = slice[0]['avg']
        print "ram_runtime = %f" % (ram_runtime)
    except:
        pass

    # GET LOCAL DISK NUMBERS
    conds['dsk'] = 'local'
    local_runtime = None
    slice = [ i for i in db if in_slice(i,conds) ]
    if DEBUG:
        print "========== local_runtime slice ================="
        print slice
        print "---------------------------------------------"
    try:
        local_runtime = slice[0]['min']
        print "local_runtime = %f (%f)" % (local_runtime, local_runtime/ram_runtime)
    except:
        pass


    # FROM HERE ON, ALL NUMBERS ARE RAM-DISK ONES
    del conds['type']
    conds['dsk'] = 'ram'                         
    print "conds = %s" % (conds)

    slice = [ i for i in db if in_slice(i,conds) ]
    if DEBUG:
        print "========== ram slice ================="
        print slice
        print "---------------------------------------------"
    n_peer_range = get_range('#peers', slice)
    print '#peers\t' + '\t\t'.join([ "%d" % n_peers for n_peers in n_peer_range ])
    ssh_estimate = []
    for n_peers in n_peer_range:
        estimate = cs_rtt_component + n_peers * lb_runtime
#        estimate = cs_rtt_component + lb_runtime + ( max(0, (n_peers - 1) * lb_runtime - (cs_rtt_component/10) ) ) + 1.01 ** n_peers
        print estimate
        ssh_estimate.append(estimate)

    print "ssh_estimate = %s" % (ssh_estimate)
    plots.append(ax.loglog(n_peer_range, [d/ram_runtime for d in ssh_estimate], line_formats.pop(), linewidth=lwidth.pop(), markersize=msize.pop(), basey=2, basex=2))
#    plots.append(ax.loglog(n_peer_range, [d/ram_runtime for d in ssh_estimate], 'r--', linewidth=2, markersize=msize, basey=2, basex=2))
    plotnames.append("sshfs est.")

    typs = ['sshfs', 'demand']

    for typ in typs:
        conds['type'] = typ
        slice = [ i for i in db if in_slice(i,conds) ]
        if DEBUG:
            print "========== multi node %s slice   =================" % (conds['type'])
            print slice
            print "---------------------------------------------"
        try:
            slice = sorted(slice, key=itemgetter('#peers'))
            plots.append(ax.loglog([d['#peers'] for d in slice], [d['avg']/ram_runtime for d in slice], line_formats.pop(), linewidth=lwidth.pop(), markersize=msize.pop(), basey=2, basex=2))
            plotnames.append(conds['type'])
            print "%s\t" % (conds['type'])  + '\t\t'.join([ "%0.2f" % (i['avg']/ram_runtime) for i in slice ])
        except:
            pass
         
    conds['type'] = 'profile'

    # GENERATE GRAPH FOR STANDARD PARAMETERIZATION
    conds['div'] = 300
    conds['btw'] = 160

    for p in [1000, 1]:
        conds['p'] = p
        slice = [ i for i in db if in_slice(i,conds) ]
        if DEBUG:
            print "========== multi node %s slice   =================" % (conds['type'])
            print slice
            print "---------------------------------------------"
        try:
            slice = sorted(slice, key=itemgetter('#peers'))
            plots.append(ax.loglog([d['#peers'] for d in slice], [d['avg']/ram_runtime for d in slice], line_formats.pop(), linewidth=lwidth.pop(), markersize=msize.pop(), basey=2, basex=2))
            plotnames.append(conds['type'])
            print "%s %d\t" % (conds['type'], conds['p'])  + '\t\t'.join([ "%0.2f" % (i['avg']/ram_runtime) for i in slice ])
        except:
            pass

    del conds['p']

    del conds['type']
    del conds['dsk']
    del conds['trial']


    # SET UP GRAPH
#    plt.axis([1, max(n_peer_range), 1, 128])
    plt.axis([1, 128, 1, 128])
    axis_limits = list(ax.axis())    
    print "ax.axis() = %s" % list(ax.axis())    
    print "Axis limits are %s" % (axis_limits)

    # ADD THE LOCAL DISK PLOT, IF AVAILABLE
    if not local_runtime is None:
        plots.append(ax.loglog(axis_limits[:2], [ local_runtime/ram_runtime, local_runtime/ram_runtime ], \
                                    line_formats.pop(), linewidth=lwidth.pop(), markersize=msize.pop(), basey=2, basex=2))
        plotnames.append('local')    
    else:
        line_formats.pop()

    # SET THE LEGEND
    if makeLegend:
        ax.legend([p[0] for p in plots], ['demand (predicted)', 'demand', 'p2p+demand', 'profile+p2p+demand 1000', 'profile+p2p+demand 1' , 'local disk'] , loc='2')

    # SET THE TITLE
    title = ''.join([k+'-'+str(v)+'-' for (k,v) in conds.items()])
    graph_title = title
    #plt.title(graph_title, fontsize=9)

    # SET LABELS AND FORMAT FOR X AND Y-AXES 
    plt.xlabel('#nodes', fontsize=20)
    plt.ylabel('normalized runtime', fontsize=20)
    ax.xaxis.set_major_formatter( mpl.ticker.ScalarFormatter() )
    ax.yaxis.set_major_formatter( mpl.ticker.ScalarFormatter() )

    # NAME OUTPUT FILE
    pdf_name  = title + '.pdf'
    print "showing %s" % (pdf_name)
    print "--------------------------------------------------------------------------"
    if doShow:
        plt.show()            
    if doSave:
        plt.savefig(pdf_name, bbox_inches='tight', pad_inches=0.01)
#        plt.savefig(pdf_name)
        plt.clf()    



### MAIN PROGRAM STARTS HERE ###

# PARSE ALL INPUT FILES
if len(sys.argv) == 1:
    print "usage: parse_summaries.py summary1 [summary2 ...]"
    print "e.g.   parse_summaries.py `ls *.summary | xargs echo -n`"
    exit(1)

for arg in sys.argv[1:]:
    parse_file(arg)

to_del = []
# CLEAN DB ENTRIES
for i in db:

    try:
        trial_type = i['type']
    except:
        print "error processing entry = %s" % (i)
        continue

    # FIELDS WE DON'T USE
    try:     
        del i['#nodes']
        del i['#servers']
        del i['typ']
        del i['d-avg']  
        del i['d-max']
        del i['d-min']
        del i['d-std']
    except:  pass

    # REMAP SIMILAR PARAMETERS
    try: 
        if i['grc'] == 1 :
            i['grc'] = 0
    except:
        pass

    # FIELDS THAT ONLY MATTER FOR PROFILE
    if i['type'] != 'profile':
        try:     del i['div']
        except:  pass
        try:     del i['btw']
        except:  pass
        try:     del i['p']
        except:  pass

    # FIELDS THAT ONLY MATTER FOR PROFILE, DEMAND
    if i['type'] != 'profile' and i['type'] != 'demand':
        try:     del i['grc']
        except:  pass

    # FIELDS THAT MATTER TO ALL BUT BINDFS
    if i['type'] == 'bindfs':
        try:     del i['stg']
        except:  pass

    # FIELDS THAT MATTER TO ALL BUT C/S (ONE PEER)
    if i['#peers'] == 1:
        try:     del i['stg']
        except:  pass
        try:     del i['grc']
        except:  pass
        
    i['avg'] = round(i['avg'])
    i['min'] = round(i['min'])
    i['max'] = round(i['max'])
    i['std'] = round(i['std'])


# GET ALL KEYS IN DB
dbkeys = {}
for keylist in [ i.keys() for i in db ]:
    for key in keylist:
        dbkeys[key] = 1

dbkeys = dbkeys.keys()
print "Avaliable keys: %s" % dbkeys

# FIND RANGES OVER WHICH WE WILL SLICE
vm_range = get_range('vm', db)
print "vmdisks=%s" % (vm_range)

conds = {}

if TEST or makeLegend:
    conds['vm'] = 'fedora2'    
    conds['tsk'] = 'noop'      
    conds['stg'] = 0 
    conds['grc'] = 7200
    plot_size_vs_runtime(conds)
    exit(0)


for vm in vm_range:
    conds['vm'] = vm
    for tsk in get_range('tsk', [ d for d in db if in_slice(d, conds)]):
        
        ## ADD TSK TO CONDITIONS
        conds['tsk'] = tsk

        for stg in get_range('stg', [ d for d in db if in_slice(d, conds)]):
            
            if stg == 2:
                continue

            conds['stg'] = stg

            for grc in get_range('grc', [ d for d in db if in_slice(d, conds)]):

                conds['grc'] = grc
                print "vm = %s\ttsk = %s\tstg = %s\tgrc = %s" % (vm, tsk, stg, grc)
                plot_size_vs_runtime(conds)
    
            del conds['grc']
        del conds['stg']
    del conds['tsk']
del conds['vm']


