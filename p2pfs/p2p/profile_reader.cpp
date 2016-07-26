

#include <iostream>
#include <fstream>
#include <iterator>
#include <exception>



#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent.hpp"
#include "libtorrent/disk_buffer_holder.hpp"

#include <pthread.h> 
#include <map> 
#include <list>
#include <stdarg.h>
#include <sstream>

#include "p2p_interface.hpp"
#include "p2p_wrapper.hpp"

using namespace libtorrent;
using namespace std;

int main(int argc, char* argv[]) {
    
  char *vmdk = NULL;
  char *profile = NULL;
  bool showbuf = false;
  
  char opt; 
  while((opt = getopt(argc, argv, "v:p:b"))!=-1) {
    switch(opt) {
    case 'v':
      cout << optarg << endl;
      vmdk = strdup(optarg);
      break;

    case 'p':
      cout << optarg << endl;
      profile = strdup(optarg);
      break;
       
    case 'b':
      cout << "showbuf" << endl;
      showbuf = true;
      break;
    }
  }
   
  if (vmdk == NULL || profile == NULL) {
    cout << "Usage: ./profile_reader -v vmdk -p profile [-b]" << endl;
    return 1;
  }
  
  ifstream ifs(profile);
  string line;
  string index_str; 
  string timestamp_str; 
  int fd = open(vmdk, O_RDWR);
  if (fd < 0){
    cout << "Error opening " << vmdk << endl;
    return -1;
  }
  char buf[BT_BLOCK_SIZE];
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double start = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;

  while(getline(ifs,line)) {
    if (line.size() < 1) {
      cout << "p2p_interface::add_profile: line size too small" << endl; 
      continue;
    } 
    
    //get the first two strings from the line: 
    stringstream st(line);
    if(!st.good() ||
       !(st >> index_str) || 
       !(st >> timestamp_str)) {
      cout << "p2p_interface::add_profile: problem w/ stringstream" << endl; 
      continue; 
    } 
    
    int index = atoi(index_str.c_str()); 
    double seconds = (double)(atof(timestamp_str.c_str()));
    off_t pos = (off_t)index * BT_BLOCK_SIZE;
    gettimeofday(&tv, NULL);
    double now = (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
    now -= start;
    double diff = seconds - now;

    cout << "index=" << index << " pos=" << pos << " seconds=" << seconds << " now=" << now; 
    if (diff > 0){
      cout << " sleep=" << diff << endl;
      usleep(diff * 1000000);
    }
    else
      cout << endl;

    int res = pread(fd, buf, BT_BLOCK_SIZE, pos);
    if (fd < 0){
      cout << "Error " << res << " reading " << vmdk << endl;
      return -1;
    } 
    
    if (showbuf)
      cout << buf << endl;

  }

  return 0;
}
  

