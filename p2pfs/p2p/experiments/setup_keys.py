"""
Add ssh keys to all servers. This script is to be executed right after all the machines are provisioned.
Also, make sure to add the public key to authorized_keys in each server.

Author: Kunal Mahajan
mkunal@cs.columbia.edu

Usage: python setup_keys.py ./keys ./server_ips.txt
"""

import os
import subprocess
import sys
from datetime import datetime

server_ips = []
USERNAME = "mkunal"

def read_server_ips(ips_file):
	with open(ips_file, "r") as fd:
		for ip in fd:
			server_ips.append(ip.strip())

def main():
	num_args = len(sys.argv)
	if num_args != 3:
		print "usage: python setup_keys.py keys_dir server_ips.txt"
		return
	read_server_ips(sys.argv[2])
	keys_dir= sys.argv[1]
	
	for ip in server_ips:
		address = USERNAME+'@'+ip+':'+'~/.ssh/'
		subprocess.call(['scp', keys_dir+'/id_rsa.pub', keys_dir+'/id_rsa', address])		

if __name__ == "__main__":
	main()