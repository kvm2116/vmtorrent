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
#include <vector>
#include <stdarg.h>

#include "vmtorrent_defines.hpp"

#ifndef  __P2P_INTERFACE_HPP_ 
#define  __P2P_INTERFACE_HPP_  1

using namespace libtorrent;


#define USE_THREADS 1 

#define P2P_OUT								\
	pthread_mutex_lock(&print_lock);				\
	{								\
		pthread_t tid = pthread_self();				\
		struct timeval tv;					\
		int nice = getpriority (PRIO_PROCESS, 0);		\
		gettimeofday(&tv, NULL);				\
		p2p_out.setf(std::ios::fixed,std::ios::floatfield);     \
		p2p_out << std::setprecision(6) << std::setbase(10)     \
			<< ((double)tv.tv_sec +				\
			    (double)tv.tv_usec/1000000) << ":"		\
			<< std::setbase(16) << "0x" << tid << ":"	\
			<< std::setbase(10) << nice << ":"		\
			<< __FILE__ << ":"				\
			<< __LINE__ << ":" << __func__ << ":\t";	\
	}								\
	p2p_out

#define P2P_ENDL std::endl ; pthread_mutex_unlock(&print_lock)

#ifdef VMTORRENT_LOCK_STATS
#define P2P_TORRENT_LOCK(piece)						\
	boost::shared_ptr<torrent> t;					\
	{								\
		struct timeval tv;					\
		gettimeofday(&tv, NULL);				\
		double start =						\
			(double)tv.tv_sec + (double)tv.tv_usec/1000000; \
		t = h.get_torrent().lock();				\
		gettimeofday(&tv, NULL);				\
		double end =						\
			(double)tv.tv_sec + (double)tv.tv_usec/1000000; \
		P2P_OUT << "obtained h.get_torrent().lock()"		\
			<< ",piece=" << piece				\
			<< ",delay=" << (end-start) << P2P_ENDL;	\
	}
#else
#define P2P_TORRENT_LOCK(piece)	boost::shared_ptr<torrent> t=h.get_torrent().lock()
#endif

#ifdef VMTORRENT_LOCK_STATS
#define P2P_PTHREAD_MUTEX_LOCK(req_lock_r,piece)			\
	{								\
		struct timeval tv;					\
		gettimeofday(&tv, NULL);				\
		double start =						\
			(double)tv.tv_sec + (double)tv.tv_usec/1000000; \
		pthread_mutex_lock(req_lock_r);				\
		gettimeofday(&tv, NULL);				\
		double end =						\
			(double)tv.tv_sec + (double)tv.tv_usec/1000000; \
		P2P_OUT << "obtained pthread_mutex"			\
			<< ",mutex=" << req_lock_r			\
			<< ",piece=" << piece				\
			<< ",delay=" << (end-start) << P2P_ENDL;	\
	}
#else
#define P2P_PTHREAD_MUTEX_LOCK(req_lock_r,piece) pthread_mutex_lock(req_lock_r)
#endif


typedef std::multimap<std::string, torrent_handle> handles_t;

//block request by a thread
//may contain additional types in the future
typedef struct block_request_ {
    block_request_ (int index, pthread_t t, char **pbuffer, int bufsize) {
	block_index = index; 
	tid =t;
	buffer = *pbuffer; 
	buffer_size = bufsize; 
	bytes_written = 0;
    }

    int block_index; //index of the block
    int buffer_size; //size of the allocated buffer
    pthread_t tid; //id of the calling thread 
    char *buffer; //allocated buffer by the calling thread
    int bytes_written; //number of bytes written to the buffer 
} block_request; 
//typedef struct block_request_ block_request;

//request params contains the locking parameters on a request entry
typedef struct request_params_ {
    request_params_() {
	pthread_cond_init(&cond, NULL);
	completed = false; 
    } 
    ~request_params_() {
	pthread_cond_destroy(&cond);
    }

    bool completed; //is request completed 
    pthread_cond_t cond; 

} request_params; 

//request entry acts as a handle for the request 
//returned to the calling thread. 
typedef struct request_entry_ {
    
    request_entry_(request_params *rpn, block_request *brn, int num_reqs = 0) {
	rp = rpn; 
	br = brn;
	num_requests = num_reqs;
    } 
    block_request *br; //pointer to a specific block_request from the calling thread
    request_params *rp; 
    
    int num_requests; //num requests for the piece : this is a copy of num_requests from piece_requests_ This may not always contain the correct value.
} request_entry;

typedef std::list<block_request*> breqs; 

typedef struct piece_requests_ {

    int num_requests; 
    breqs brequests; 
    request_params *rp; 

    piece_requests_();
    ~piece_requests_();
    request_entry insert(block_request *br);
    request_entry find(block_request *br);

    //tries to remove block request, returns the number of block requests for
    //the same piece remaining
    int remove(block_request *br);
    request_entry copy_buffer(peer_request r, disk_io_job const& j);
} piece_requests; 



typedef std::map<int,piece_requests*> request_map; 

class RequestsHash {
public:

    request_map requests; 

    request_entry insert(block_request *br);

    request_entry find_by_block_index(int block_index);

    request_entry find(block_request *br);

    void remove(block_request *br);

    //copies the arrived piece buffer into the buffer of every block request
    //waiting for this piece. 
    //returns request entry for this piece' request 
    request_entry copy_buffer(peer_request r, disk_io_job const& j);
}; 


typedef struct profile_entry_ {
    profile_entry_(int i, int min_t, int mean_t, double f) {
	piece_index = i; 
	min_access_time = min_t; 
	mean_access_time = mean_t; 
	freq = f;
    } 
    int piece_index; //index of a block under the 16KB indexing
    double min_access_time, mean_access_time, freq;
} profile_entry; 

typedef std::vector<profile_entry> profile; 

typedef struct piPtr_int_ {
    void *piPtr;
    int val;
} piPtr_int;


#define IDLE 0
#define READ_IN_PROG 1
#define WAITING_TO_ISSUE_READ 2

class p2p_interface
{
private: 
    torrent_handle h;
    session ses;
    RequestsHash requests; 
    pthread_mutex_t req_lock; 
    pthread_mutex_t priority_lock; 
    pthread_mutex_t print_lock;
    double *priority_request_time;
    short *current_piece_priority;
    bool *piece_on_disk;
    char *read_status;
    int on_demand;
    std::ofstream p2p_out;

public:
    profile pending_profile; 
    profile active_profile; 
    bool torrent_started; 
    int num_pieces; 
    int piece_length;
    long total_size; 
    int last_piece_size; 
    
    int blocks_per_piece; //# of 16KB blocks per piece 
    int last_block_index; //index of the last block 
    int last_block_size;  //size of the last block in the 16KB system 
    
    int high_rate_limit;
    int low_rate_limit;
    int random_diversity_window; //the number of pending profile entries from which the next request may be selected
    int bt_profile_window; //the number of allowed in-flight profile entries

#ifdef VMTORRENT_PUSH
    void* (*p2p_push)(void *piece);
#endif

    std::map<const char *, const char *> peers;
    p2p_interface();
    ~p2p_interface();

    void set_piece_priority(int index, int priority);

    torrent_info const& get_torrent_info();

    //API function to start torrent. A blocking call. Returns when the torrent is started and is being downloaded 

    int start_torrent(const char *torrent_file, const char *dir);

    //same as above, but providing the port range to run on
    int start_torrent(const char *torrent_file, const char *dir, 
		      int min_port, int max_port);
    
    int start_torrent(const char *torrent_file, const char *dir, int demand, int upload_limit, int download_limit);
    
    //internal call, made by start_torrent
    int start_torrent_(const char *torrent_file, const char *dir, int min_port, 
		       int max_port, int demand, int upload_limit, 
		       int download_limit);
    
    //adds a dht node one can use to boostrap
    //void add_dht_node(std::pair<std::string, int> const& node);
    void add_dht_node(const char *host, int port);

    //add a peer to talk to:
    void add_peer(const char *host, int port);
    
    //here port is in the string format
    void add_peer(const char *host, const char *port);
    
    //API function add_peer_file adds a file in the format:
    //<host:port> 
    //<host:port>
    //...
    //if port is omitted, the default 6881 is used 
    int add_peer_file(const char *filename); 
    
    //API function add_profile                                                
    //takes a pointer to a profile file that contains       
    //two words per line: <16KB block index> <first access timestamp>
    //if port is omitted, the default 6881 is used  
    int add_profile(const char *filename, int priority); 
    
    //implements the logic used to determine which profiled blocks will be requested now
    int apply_profile(int width, int priority); 

    //inits all pieces to input priority 
    int init_priorities(int priority); 
    
    //if any peers were specified by add_peer or add_peer_file, 
    //connects to those peers 
    void connect_to_peers(); 
    
    //API function get_block 
    //This function is called by a thread that wants to read in a given piece
    //Into a pre-allocated buffer. This call will block until the data is
    //Read in.
    //Returns -1 on error, or an integer <= bufsize with the number of bytes_written
    int get_block(int block_index, char**pbuffer, int bufsize);

    int read_from_piece(int piece_index, int piece_offset, int block_index);
    //callback from libtorrent when a piece is fully downloaded and verified
    //for its content 

    static void piece_downloaded_wrapper(void *piPtr, int piece_index) {
	p2p_interface *p = (p2p_interface*) (piPtr); 
	p->piece_downloaded(piece_index);
	pthread_t thread;
	piPtr_int *pi = new piPtr_int;
	pi->piPtr = piPtr;
	pi->val = piece_index;
	pthread_create(&thread, NULL, check_profile_wrapper, (void*) pi);
    } 

    static void *check_profile_wrapper(void *vi) {
	piPtr_int *pi = (piPtr_int *) vi; 
	p2p_interface *p = (p2p_interface*) (pi->piPtr) ; 
	p->check_profile( (long) (pi->val) );	
	delete pi;
    }
    
    void piece_downloaded(int piece_index);

    void check_profile(int piece_index);

    //asynchronous callback from the libtorrent's disk_io_thread. 
    //j contains the buffer, that is automatically deallocated when 
    //disk_buffer_holder is off the stack 
    //r contains the requet object with which async_read was called 
    void on_disk_read_complete(int ret, disk_io_job const& j, peer_request r);

    void print_alert(alert const* a, std::ostream& os);
    void handle_alert(session& ses, alert* a);
    //void handle_alert(session& ses, alert* a, handles_t const& handles);

    //run code for all alerts in a loop
    void *alert_loop(void *vp);
}; 

#endif
