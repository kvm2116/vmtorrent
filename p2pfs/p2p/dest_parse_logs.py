import os
import sys

LOGFILE = ''

def main():
    global LOGFILE
    directory = "/home/bw2387/test_scripts/libtorrent_logs6881"
    print sys.argv[1]
    LOGFILE = sys.argv[1]
    analyze_files(directory)
    parse_log()

def analyze_files(root, depth = 0):
    #print("  " * depth + root)

    if (not os.path.isdir(root)):
        return
    print root
    if (len(root) > 0 and root[-1] != os.sep):
        root += os.sep
    entries = os.listdir(root)
    for entry in entries:
        entry_path = root + entry
        if (entry.endswith('.log')):
            add_to_log(entry_path)
        analyze_files(entry_path, depth + 1)

def add_to_log(file_name):
    from_file = open(file_name, 'r+')
    to_file = open(LOGFILE, 'a')

    to_file.write('LOG FILE*********\n')
    to_file.write(file_name+'\n')


    if "main_session" in file_name:
        to_file.write("PIECES******\n")
        connection = ""
        pieces = 0
        for line in from_file:
            if "ON_CONNECT" in line or "INCOMING CONNECTION" in line:
                if (line != connection and connection !="" and pieces > 0):
                    to_file.write("connection: "+connection)
                    to_file.write("pieces: "+str(pieces)+"\n")
                connection = line
                pieces = 0
        
            if "PIECE_FINISHED" in line:
                pieces+=1
        if (pieces > 0):
            to_file.write("connection: "+connection)
            to_file.write("pieces: "+str(pieces)+"\n")
        
        return

    pieces = 0
    p_lines = []
    for line in from_file:
        if "INCOMING CONNECTION" in line:
            to_file.write(line)
        elif "OUTGOING_CONNECTION" in line:
            to_file.write(line)
        elif "PIECE" in line:
            pieces += 1
            p_lines.append(line)
        elif "CONNECTION FAILED" in line:
            to_file.write(line)
    to_file.write("PIECE_LINES***\n")
    to_file.write("".join(p_lines))
    to_file.write("piece msg: "+str(pieces)+"\n")
    to_file.close()
    from_file.close()

def parse_log():
    logfile = open(LOGFILE, 'a+')
    incoming = 0
    outgoing = 0
    successful = 0
    successful_lines = []
    failed_total = 0
    failed_peer = 0

    possible_success = ""
    for line in logfile:
        if 'CONNECTION FAILED' in line:
            failed_total += 1
            if 'connected to ourselves' in line:
                failed_peer += 1
        if 'INCOMING CONNECTION' in line:
            incoming += 1
            possible_success = line
        if 'OUTGOING_CONNECTION' in line:
            outgoing += 1
            possible_success = line
        if 'piece msg' in line:
            msg = line.split(": ")
            if int(msg[1])>0:
                successful +=1
                successful_lines.append(possible_success)

    logfile.write("TOTALS********\n")
    logfile.write("incoming: "+str(incoming)+"\n")
    logfile.write("outgoing: "+str(outgoing)+"\n")    
    logfile.write("sucessful: "+str(successful)+"\n")
    logfile.write("successes: \n")
    for l in successful_lines:
        logfile.write(l)
    logfile.write("failed_total: "+str(failed_total)+"\n")
    logfile.write("failed_peer: "+str(failed_peer)+"\n")

    logfile.close()
if __name__ == "__main__":
       main()

