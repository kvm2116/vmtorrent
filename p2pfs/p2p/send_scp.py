import os
import subprocess
import sys
from datetime import datetime

USERNAME = "bw2387"
KEY = "/home/bw2387/.ssh/vmtorrent_key"
FILE = "scp_logtime.txt"

def main():
    num_args = len(sys.argv)
    src_directory= sys.argv[1]
    dst_directory = sys.argv[2]
    trial_num = sys.argv[3]
    log = open(FILE, 'a+')
    log.write("TRIAL "+trial_num+"\n")
    for i in range(4, num_args):
        now = datetime.now()
        log.write("sent to machine "+str(i)+" at: "+now.strftime("%Y-%m-%d %H:%M:%S.%f")+"\n")
        send_all_files(sys.argv[i], src_directory, dst_directory)

def send_all_files(ip, src_directory, dst_directory):
    address = USERNAME+'@'+ip+':'+dst_directory
    for filename in os.listdir(src_directory):
        subprocess.call(['scp', '-i', KEY, src_directory+'/'+filename, address])

if __name__ == "__main__":
   main()
