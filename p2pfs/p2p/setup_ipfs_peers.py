import os
import subprocess
import sys
from datetime import datetime
import time

def main():
    num_args = len(sys.argv)
    if num_args != 2:
        print "USAGE: python setup_ipfs_peers.py ipfs_peers.txt"
        return

    with open(sys.argv[1]) as pfd:
        for peer in pfd:
            address = peer.strip('\n')    
            subprocess.call(['ipfs', 'swarm', 'connect', address])

if __name__ == "__main__":
   main()
