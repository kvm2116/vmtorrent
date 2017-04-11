import os
import subprocess
import sys
from datetime import datetime

USERNAME = "bw2387"
KEY = "/home/bw2387/.ssh/vmtorrent_key"
FILE = "scp_tar_logtime.txt"

def main():
    num_args = len(sys.argv)
    src_directory= sys.argv[1]
    dst_directory = sys.argv[2]
    trial_num = sys.argv[3]
    log = open(FILE, 'a+')
    log.write("TRIAL "+trial_num+"\n")
    tar_name = src_directory+'.tar.gz'
    subprocess.call(['tar', '-cvzf', tar_name, src_directory])
    for i in range(4, num_args):
        now = datetime.now()
        log.write("sent to machine "+str(i)+" at: "+now.strftime("%Y-%m-%d %H:%M:%S.%f")+"\n")
        send_tar_file(sys.argv[i], tar_name, dst_directory)
def send_tar_file(ip, tar_name, dst_directory):
    address = USERNAME+'@'+ip+':'+dst_directory
    subprocess.call(['scp', '-i', KEY, tar_name, address])

if __name__ == "__main__":
   main()
