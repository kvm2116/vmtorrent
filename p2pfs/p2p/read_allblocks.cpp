

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

#include "p2p_interface.hpp"

using namespace libtorrent;

int main(int argc, char* argv[]) {
    
    char torrent_file[1000]; 
    char storage_dir[1000]; 
    char seed_host[1000]; 
    char peer_file[1000];
 
    int seed_port =-1; 
    int local_port = -1; 

    bool torrent_given = false; 
    bool directory_given = false; 
    bool seed_given = false; 
    bool peer_file_given = false; 

    char *ptr; 

    char opt; 
    while((opt = getopt(argc, argv, "t:d:s:p:e:"))!=-1) {
        switch(opt) {
	case 't':
	    strcpy(torrent_file, optarg); 
	    torrent_given = true; 
	    break;
 
	case 'd':
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

	case 'e':
	  strcpy(peer_file, optarg); 
	  peer_file_given = true; 
	  break; 
	}
    }

    
    if (!directory_given || !torrent_given) {
	printf("Usage: ./p2p_test -t <.torrent file> -d <storage directory> [-s <seed hostname>] [-p <seed port>]\n"); 
	return 1;
    }

    if (seed_port > 0) {
	printf("Seed port is defauled to 6881, so -p arguments ignored\n"); 
    } 

    //log_init();
    
    p2p_interface *pi = new p2p_interface();

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
    int ret; 
    if (local_port > 0) { 
      ret = pi->start_torrent(torrent_file, storage_dir, local_port, local_port);
    } 
    else {
      ret = pi->start_torrent(torrent_file, storage_dir);
    } 

    //log_line(LOG_INFO, LOG_PARAMS, "Torret START success with %d pieces of size: %d. total_size: %d last_piece_size: %d\n", pi->num_pieces, pi->piece_length, pi->total_size, pi->last_piece_size);
    
    std::cout << "TORRENT STARTED" << std::endl;

    if (peer_file_given) { 
      pi->add_peer_file(peer_file); 
    } 
    

    sleep(1); 
    
    //add a DHT node to another peer: this allows the client to run 
    //in a tracker-less mode.

    
    system("touch copy.tmp");
    int fd  = open("copy.tmp", O_WRONLY); 
    if (fd<0) {
	printf("ERROR fopen \n"); 
	exit(1); 
    } 
    
    int num_pieces = pi->num_pieces; 
    int blocks_per_piece = pi->blocks_per_piece; 
	
    //BT_BLOCK_SIZE is 16*1024
    char *block = (char*)malloc(BT_BLOCK_SIZE); 
    if (!block) {
	printf("ERROR malloc \n"); 
	exit(1); 
    } 
    
    for(int i=0; i<num_pieces; i++) { 
      
      for(int j=0; j<blocks_per_piece; j++) { 
	//tries to read the block at index 0
	//returns -1 on error or bytes_written as number of bytes copied into 
	//the buffer. 
	//On success, 
	//bytes_written should always be BT_BLOCK_SIZE, unless it's the last block
	//and a smaller number will be returned.
 
	int block_index = blocks_per_piece*i+j;
	if (block_index > pi->last_block_index) {
	  break;
	} 

	//log_line(LOG_INFO, LOG_PARAMS, "READ_ALLBLOCKS: about to request piece: %d block: %d. Translating to block_index: %d\n", 
	//	 i, j, block_index);

	int bytes_written = pi->get_block(block_index, &block, BT_BLOCK_SIZE);
	
	//std::cout << "block: " << block_index << " bytes_written: " << bytes_written << std::endl;

	int to_write = bytes_written;
	int offset = 0; 
	while(to_write > 0) { 
	  int ret = write(fd, block+offset, to_write);
	  if (ret < 0) {
	    printf("ERROR write\n"); 
	  } 
	  to_write -=ret;
	  offset +=ret; 
	} 
	
      }//for
    }
    close(fd); 
    free(block); 
    
    //runs forever... 
    while(1) {
      sleep(1000); 
    } 
    
    return 0;
}


