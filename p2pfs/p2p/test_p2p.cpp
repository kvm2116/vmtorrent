

#include <iostream>
#include <fstream>
#include <iterator>
#include <exception>
#include <boost/filesystem.hpp>


#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent.hpp"
#include "libtorrent/disk_buffer_holder.hpp"

#include <pthread.h> 
#include <map> 
#include <list>
#include <stdarg.h>
#include <string>

#include "p2p_interface.hpp"
#include "p2p_wrapper.hpp"


using namespace libtorrent;
//using std::tr2::sys;
using namespace std;
using namespace boost::filesystem;

bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char* argv[]) {
    
    char torrent_dir[1000]; 
    char storage_dir[1000]; 
    char seed_host[1000]; 
    char peer_file[1000]; 
    char library_path[1000]; 
    char profile_file[1000]; 
    vector<string> torrent_files;

    int seed_port =-1; 
    int local_port = -1; 
    int upload_limit = 0;
    int download_limit = 0;
    
    bool torrent_dir_given = false;
    bool torrent_given = false; 
    bool directory_given = false; 
    bool seed_given = false; 
    bool peer_file_given = false; 
    bool read_all_blocks = false; 
    bool profile_given = false; 

    char *ptr = NULL; 
    int fid; 

    char opt;

    while((opt = getopt(argc, argv, "t:l:s:p:e:u:d:ro:f:"))!=-1) {
        switch(opt) {
	case 't':
	    strcpy(torrent_dir, optarg); 
	    torrent_dir_given = true; 
	    break;
	    
	case 'l':
	    strcpy(storage_dir, optarg);
	    directory_given = true; 
	    break; 
	    
	case 's':
	    seed_given = true; 
	    ptr = strchr(optarg, ':'); 
	    if (!ptr) { 
		strcpy(seed_host, optarg);
	    }
	    else {
		strncpy(seed_host, optarg, ptr-optarg);
		seed_host[ptr-optarg]=0; 
		seed_port = atoi(ptr+1);
	    } 
	    break;
	    
        case 'p':
            local_port = atoi(optarg); 
            break; 

        case 'u':
            upload_limit = atoi(optarg); 
	    upload_limit = upload_limit * (1 << 7);
            break; 

        case 'd':
            download_limit = atoi(optarg); 
	    download_limit = download_limit * (1 << 7);
            break; 

	case 'e':
	  strcpy(peer_file, optarg); 
	  peer_file_given = true; 
	  break; 
	
	case 'r':
	    read_all_blocks = true;
	    break;
	    
	case 'o':
	    //redirect output to a given file 
	    fid = open(optarg, O_RDWR|O_CREAT|O_TRUNC, 0666); 
	    if (fid == -1) {
	      std::cout << "Failed to open log file: " <<  optarg << std::endl; 
		exit(1); 
	    } 
	    //close(1); 
	    dup2(fid, 1); 
	    std::cout << "Redirecting stdout to " << optarg << std::endl;; 
	    break;
	    
	case 'f':
	    //read a given profile
	    strcpy(profile_file, optarg); 
	    profile_given = true; 
	    break; 
	}

    }

    if (torrent_dir_given) {
        path p (torrent_dir);
	if (is_directory(p))
  	{
    	    for (directory_iterator itr(p); itr!=directory_iterator(); ++itr)
            {
                string fname = itr->path().filename().string();
                cout << fname << ' '; // display filename only
                if (is_regular_file(itr->status())) cout << " [" << file_size(itr->path()) << ']';
                 cout << '\n';
                if (has_suffix(fname, ".torrent")) {
                    torrent_files.push_back(itr->path().filename().string());
                    cout << "torrent file\n";
                }
            }
        }
    }

    cout << "myvector contains:";
    for (std::vector<string>::iterator it = torrent_files.begin() ; it != torrent_files.end(); ++it)
        std::cout << ' ' << *it;
    std::cout << '\n';

    if (!directory_given || !torrent_given) {
      std::cout << "Usage: ./p2p_test -t <.torrent file> -l <storage directory> [-u upload_limit] [-d download_limit] [-e <peer file>] [-r] [-o output_file] [-f profile] '[-p <local port to run on, default:6881>]" <<std::endl; 
      return 1;
    }

    char torrent_file[1000];
 
    std::cout << "p2p_client with args: " <<std::endl;
    if (torrent_given)
       std::cout << "\ttorrent= " << torrent_file <<std::endl;
    std::cout << "\tlocal_port= " << local_port <<std::endl;
    std::cout << "\tupload limit= " << upload_limit <<std::endl;
    std::cout << "\tdownload_limit= " << download_limit <<std::endl;
   
    p2p_interface *pi = new p2p_interface();
    
    //add a DHT node to another peer: this allows the client to run 
    //in a tracker-less mode. 
    if (seed_given) { 
      if (seed_port > 0) {
	pi->add_peer(seed_host, seed_port); 
      } 
      else { 
	pi->add_peer(seed_host, 6881); 
      }
    }
    
    sleep(1); 

    //start_torrent is currently a blocking call, and will return only when
    //the torrent is started successfully 
    int ret = -1; 
    if(local_port > 0) {
      ret = pi->start_torrent_(torrent_file, storage_dir, local_port, local_port, 0, upload_limit, download_limit);
    }
    else {
      ret = pi->start_torrent(torrent_file, storage_dir, 0, upload_limit, download_limit);
    } 

    std::cout << "TORRENT STARTED" << std::endl;

    // READ BUT DON'T CALL ADD PROFILE
    if (profile_given) {
      std::cout << "Using profile " << profile_file << std::endl;
      pi->add_profile(profile_file, PROFILE_CLOSE_PRIORITY); 
    } 

    if (peer_file_given) { 
      std::cout << "Using peers from " << peer_file << std::endl;
      pi->add_peer_file(peer_file); 
    } 

    sleep(1); 
    
    char *block = (char*)malloc(BT_BLOCK_SIZE); 
    int bytes_written = 0; 

    // MAKE ANOTHER OPTION CALLED SIMULATE OR SOMETHING
    // THAT READS BLOCKS AS BELOW, BUT BASED ON PROFILE AND TIMING
    if (read_all_blocks) {
      for(int i=0; i<=pi->last_block_index; i++) {
	      pi->set_piece_priority(i/pi->blocks_per_piece, DEMAND_PRIORITY);
	      bytes_written = pi->get_block(i, &block, BT_BLOCK_SIZE);
	      std::cout<< "get_block: " << i << " returned: " << bytes_written << std::endl;
	}
    }
    else { 
	//tries to read the block at index 0
	//returns -1 on error or bytes_written as number of bytes copied into 
	//the buffer. 
	//On success, 
	//bytes_written should always be BT_BLOCK_SIZE, unless it's the last block
	//and a smaller number will be returned. 
	bytes_written = pi->get_block(0, &block, BT_BLOCK_SIZE);
	
	std::cout << "get_block: " << 0 << " returned: " << bytes_written << std::endl;
    }
    //runs forever... 
    while(1) {
	sleep(1000); 
    } 

    return 0;
}


