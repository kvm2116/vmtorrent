import os
import subprocess
import sys
from datetime import datetime
import time

USERNAME = "mkunal"
KEY = "/home/mkunal/.ssh/vmtorrent_key"
FILE = "rsync_logtime.txt"
log = open(FILE, 'w+')

def main():
    num_args = len(sys.argv)
    src_directory= sys.argv[1]
    dst_directory = sys.argv[2]
    trial_num = sys.argv[3]
    log.write("TRIAL "+trial_num+"\n")
    for i in range(4, num_args):
        now = datetime.now()
        ip = sys.argv[i]
        log.write("\n")
        log.write("sent to machine "+ ip + " at: " + now.strftime("%Y-%m-%d %H:%M:%S.%f")+"\n")
        send_all_files(ip, src_directory, dst_directory)
    log.close()

def send_all_files(ip, src_directory, dst_directory):
    address = USERNAME+'@'+ip+':'+dst_directory
    #for filename in os.listdir(src_directory):
    #    subprocess.call(['scp', '-i', KEY, src_directory+'/'+filename, address])
    start_time = time.time()
    subprocess.call(['rsync', '-avzhe', 'ssh', src_directory, address])
    elapsed_time = time.time() - start_time # elapsed time in seconds
    log.write("Time for transfer : ")
    log.write(str(elapsed_time))
    log.write(" seconds\n")

if __name__ == "__main__":
   main()
