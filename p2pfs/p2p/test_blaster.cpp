

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

pthread_mutex_t CompareFileLock;
bool CheckReadBytes = false; 
char ComparisonFile[300] = "tmp.out"; 
static int NumThreadTests = 0; 

using namespace libtorrent;

typedef struct test_struct_ {
    test_struct_(p2p_interface *pin, int index_new) {
	pi = pin;
	index = index_new; 
    } 

    p2p_interface *pi; 
    int index; 
    int min_index; 
    int max_index; 
    
} test_struct; 


int compare_mem_buffers(char **p2p_buffer, int offset, int bufsize) {

    int ret = -1; 

    char *buf = (char*)malloc(bufsize); 
	
   //log_line(LOG_INFO,//LOG_PARAMS, "Calling compare_mem_buffers.  Thread: %u\n", pthread_self());
    pthread_mutex_lock(&CompareFileLock); 
    FILE *f = fopen(ComparisonFile, "r");

   //log_line(LOG_INFO,//LOG_PARAMS, "compare_mem_buffers: file_opened.  Thread: %u\n", pthread_self());
    fseek(f, offset, SEEK_SET); 

   //log_line(LOG_INFO,//LOG_PARAMS, "compare_mem_buffers: fseed called.  Thread: %u\n", pthread_self());
    
    int n = fread(buf, 1, bufsize, f); 
    if (n != bufsize) {
	std::cout << "fread error" << std::endl;
	pthread_mutex_unlock(&CompareFileLock); 
	exit(1); 
    } 

    NumThreadTests++; 
    pthread_mutex_unlock(&CompareFileLock); 

    fclose(f);     
    ret = memcmp((void*)buf, (void*)*p2p_buffer, bufsize); 
    free(buf); 
    
   //log_line(LOG_INFO,//LOG_PARAMS, "compare_mem_buffers: Returning: %d.  Thread: %u\n", ret, pthread_self());

    return ret; 
} 

void *test_get_piece(void *v) {
    test_struct  *ts = (test_struct*)v;
    
    for (int j=0; j<100;j++) { 
      char *block = (char *)malloc(BT_BLOCK_SIZE); 

     //log_line(LOG_INFO,//LOG_PARAMS, "Test request for block: %d. Thread: %u\n", ts->index, pthread_self());

      //pick a random block between min_index and max_index 
      int index = ts->min_index + (int) ((double)(ts->max_index-ts->min_index+1) * (rand() / (RAND_MAX + 1.0)));

     //log_line(LOG_INFO,//LOG_PARAMS, "TEST_BLASTER: About to request block: %d. Thread: %u\n"\
	       , index, pthread_self());
      //ts->pi->set_piece_priority(index,7);
      int bytes_written = ts->pi->get_block(index, &block, BT_BLOCK_SIZE);
      
      int ret; 
      if (CheckReadBytes) {
	  int len = BT_BLOCK_SIZE; 
	  if (index == ts->pi->last_block_index) {
	      len = ts->pi->last_block_size; 
	  } 
	  ret = compare_mem_buffers(&block, (BT_BLOCK_SIZE)*(index), len);
	  if (ret!= 0) {
	      std::cout << "comparison failed on read of: " << index << std::endl;
	      exit(1);
	  } 
	  else {
	     //log_line(LOG_INFO,//LOG_PARAMS, "compare_mem_buffers: Read block: %d. Bytes_written: %d\n", index, bytes_written);
	      std::cout << "Read of block: " << index << "succeeded. Read: " << bytes_written << " bytes. Starting new request " << std::endl;
	  } 
	  
      }
      
      free(block); 
    
    std::cout << "read block return: " << ret << std::endl; 
    }
}

void init_comparison_file() {
    pthread_mutex_init(&CompareFileLock, NULL); 
    
} 

void test_blaster(p2p_interface *pi) {

     
    //if we want to compare read bytes with bytes from a comparison file
    //on disk then start a comparison file: 
    if (CheckReadBytes) {
	init_comparison_file(); 
    }

    //blast 300 threads:
    int NumThreads = 300;
 
    int MaxPiece = 200; 
    test_struct *ts = new test_struct(pi, MaxPiece); 
    
    //blast requests for pieces between min_index and max_index
    ts->min_index = pi->last_block_index-200;
    ts->max_index = pi->last_block_index;
    
    if (ts->min_index < 0) {
      ts->min_index = 0; 
    } 

    pthread_t threads[NumThreads]; 
    for(int i=0; i<NumThreads; i++) {
      int ret = pthread_create(&(threads[i]), NULL, &test_get_piece, (void*)ts);
 
    } 
    for (int j=0; j<NumThreads; j++) {
      int ret = pthread_join(threads[j], NULL); 
      std::cout << "Thread " << j << " completed task " << std::endl;
      std::cout << "TOtal Number of Tests: " << NumThreadTests << std::endl;
     //log_line(LOG_INFO,//LOG_PARAMS, "Total Number of Tests: %d\n", NumThreadTests);
    } 
} 


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
    bool comparison_given = false; 
    bool peer_file_given = false; 
    
    char *ptr = NULL; 

    char opt; 
    while((opt = getopt(argc, argv, "t:d:s:p:c:e:"))!=-1) {
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

	case 'c':
	  strcpy(ComparisonFile, optarg);
	  comparison_given = true; 
	  CheckReadBytes = true;
	  break;
 
	case 'e':
	  strcpy(peer_file, optarg); 
	  peer_file_given = true; 
	  break; 
	}
    }

    printf("test_blaster with args: \n\t local_port=%d \n\t seed_name=%s \n\t seed_port=%d\n\t comparison file: %s CheckReadBytes: %d\n", 
	   local_port, 
	   seed_host, 
	   seed_port, 
	   ComparisonFile, 
	   CheckReadBytes);
    

    if (!directory_given || !torrent_given) {
	printf("Usage: ./test_blaster -t <.torrent file> -d <storage directory> [-c <file to compare the downloaded bytes against] [-s <seed hostname>] [-p <local port to run on, default:6881>]\n"); 
	return 1;
    }

    
   //log_init();
    
    p2p_interface *pi = new p2p_interface();
    
    //start_torrent is currently a blocking call, and will return only when
    //the torrent is started successfully 

    if (seed_given) { 
      if (seed_port > 0) {
	pi->add_peer(seed_host, seed_port); 
      } 
      else { 
	pi->add_peer(seed_host, 6881); 
      }
    }
    
    sleep(1); 
    
    int ret = -1; 
    if(local_port > 0) {
      ret = pi->start_torrent(torrent_file, storage_dir, local_port, local_port, 0);
    }
    else {
      ret = pi->start_torrent(torrent_file, storage_dir);
    } 

   //log_line(LOG_INFO,//LOG_PARAMS, "Torret START success with %d pieces of size: %d. total_size: %d\n", pi->num_pieces, pi->piece_length, pi->total_size);
    
    std::cout << "TORRENT STARTED" << std::endl;

    if (peer_file_given) { 
      pi->add_peer_file(peer_file); 
    } 
        
    test_blaster(pi); 
    
    sleep(1); 
    return 0;
}


