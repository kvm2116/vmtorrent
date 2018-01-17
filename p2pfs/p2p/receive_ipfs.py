import os
import subprocess
import sys
from datetime import datetime
import time

USERNAME = "mkunal"
KEY = "/home/mkunal/.ssh/vmtorrent_key"


def main():
    num_args = len(sys.argv)
    if num_args != 4:
        print "USAGE: python receive_ipfs.py <TRIAL_NUM> <IPFS_OBJ> <OUTFILE>"
        return
    
    trial_num = sys.argv[1]
    ipfs_object= sys.argv[2]
    FILE = sys.argv[3]
    log = open(FILE, "w+")
    log.write("TRIAL "+trial_num+"\n")
    log.write("\n")
    start_time = time.time()
    subprocess.call(['ipfs', 'get', ipfs_object])
    elapsed_time = time.time() - start_time # elapsed time in seconds
    log.write("Time for transfer : ")
    log.write(str(elapsed_time))
    log.write(" seconds\n")
    log.close()

if __name__ == "__main__":
   main()
