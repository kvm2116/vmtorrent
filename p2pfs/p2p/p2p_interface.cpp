#include <iostream>
#include <fstream>
#include <iterator>
#include <exception>
#include <sstream>

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent.hpp"
#include "libtorrent/disk_buffer_holder.hpp"
#include "libtorrent/alert.hpp"
#include "libtorrent/alert_types.hpp"

#include <pthread.h> 
#include <map> 
#include <list>
#include <stdarg.h>

#include "p2p_interface.hpp"

//#include "leveldb/db.h"

using namespace libtorrent;
bool compact_mode = false;
bool downloading = true;


piece_requests::piece_requests_() {
    num_requests = 0; 
    rp = new request_params;
} 
    
piece_requests::~piece_requests_() {
    delete rp;
} 

request_entry piece_requests::insert(block_request *br) {
    breqs::iterator iter; 
    for(iter = brequests.begin(); iter!= brequests.end(); iter++) {
	block_request *brold = *iter; 
	if (brold->tid == br->tid) {
	    //ALERT - trying to insert request for the same piece from the same thread
	    return request_entry(rp, brold); 
	} 
    }
    brequests.push_back(br); 
    num_requests++; 
    //this could happen: that we are inserting a request entry for a request
    //that has already been completed. In that case copy the buffer to the new request
    
    if (rp->completed  && num_requests > 1) {
	iter = brequests.begin();
	block_request *bfirst = *iter; 
	if (bfirst->bytes_written > 0) { 
	    memcpy(br->buffer, bfirst->buffer, bfirst->bytes_written);
	    br->bytes_written = bfirst->bytes_written;
	} 
    } 
    
    
    return request_entry(rp, br);
} 


request_entry piece_requests::find(block_request *br) {
    breqs::iterator iter; 
    for(iter = brequests.begin(); iter!= brequests.end(); iter++) {
	block_request *brold = *iter; 
	if (brold->tid == br->tid) {
	    //ALERT - trying to insert request for the same piece from the same thread
	    return request_entry(rp, brold, num_requests); 
	} 
    }
    return request_entry(NULL, NULL);
}

//tries to remove block request, returns the number of block requests for
//the same piece remaining
int piece_requests::remove(block_request *br) {
    breqs::iterator iter; 
    for(iter = brequests.begin(); iter!= brequests.end(); iter++) {
	block_request *brold = *iter; 
	if (brold->tid == br->tid) {
	    //ALERT - trying to insert request for the same piece from the same thread
	    brequests.erase(iter);
	    num_requests--;
	    return num_requests; 
	} 
    }
    return num_requests; 
}

request_entry piece_requests::copy_buffer(peer_request r, disk_io_job const& j) {
    breqs::iterator iter; 
    for(iter = brequests.begin(); iter!= brequests.end(); iter++) {
	block_request *br = *iter;
	int len = std::min(r.length, br->buffer_size);
	if (len > 0) { 
	    memcpy(br->buffer, j.buffer, len);
	    br->bytes_written = len;
	}
    }
    return request_entry(rp, NULL, num_requests); 
} 


//////RequestsHash
request_entry RequestsHash::insert(block_request *br){
    
    piece_requests *p; 
    request_map::iterator iter = requests.find(br->block_index);
    if (iter == requests.end()) {
	p= new piece_requests();
	requests[br->block_index] = p; 
    } 
    else {
	p = iter->second; 
    } 
    return p->insert(br);
    
}

request_entry RequestsHash::find_by_block_index(int block_index) {
    request_map::iterator iter = requests.find(block_index);
    if (iter == requests.end()) {
	return request_entry(NULL, NULL); 
    } 
    piece_requests *p = iter->second; 
    return request_entry(p->rp, NULL, p->num_requests);
} 

request_entry RequestsHash::find(block_request *br) {
    request_map::iterator iter = requests.find(br->block_index);
    if (iter == requests.end()) {
	return request_entry(NULL, NULL); 
    } 
    piece_requests *p = iter->second; 
    return p->find(br); 
} 
    
void RequestsHash::remove(block_request *br) {
    request_map::iterator iter = requests.find(br->block_index);
    if (iter == requests.end()) {
	    return; 
    } 
    piece_requests *p = iter->second; 
    int remaining = p->remove(br); 
    if (remaining ==0) {
	delete p; 
	requests.erase(iter);
    } 
} 


//copies the arrived piece buffer into the buffer of every block request
//waiting for this piece. 
//returns request entry for this piece' request 
request_entry RequestsHash::copy_buffer(peer_request r, disk_io_job const& j) {
    request_map::iterator iter = requests.find(r.piece);
    if (iter == requests.end()) {
	return request_entry(NULL, NULL); 
    } 
    piece_requests *p = iter->second;
    return p->copy_buffer(r, j);
} 

///p2p_interface 
p2p_interface::p2p_interface() {
    pthread_mutex_init(&req_lock, NULL); 
    pthread_mutex_init(&priority_lock, NULL); 
    pthread_mutex_init(&print_lock, NULL); 
    torrent_started = false; 
    p2p_out.open("p2pm.log");

#ifdef VMTORRENT_PUSH
    p2p_push = NULL;
#endif
#ifdef P2P_INTERFACE_DEBUG
    P2P_OUT << "Using P2P_INTERFACE_DEBUG" << P2P_ENDL;
#endif
#ifdef P2P_DEBUG
    P2P_OUT << "Using P2P_DEBUG" << P2P_ENDL;
#endif
    // magic numbers
    random_diversity_window = 1;
    bt_profile_window = 2; 
}
    
p2p_interface::~p2p_interface() {
    pthread_mutex_destroy(&req_lock);  
    pthread_mutex_destroy(&priority_lock);  
    pthread_mutex_destroy(&print_lock);  
    p2p_out.close();
} 


int p2p_interface::start_torrent(const char *torrent_file, const char *dir) {
    start_torrent_(torrent_file, dir, -1, -1, 0, 0, 0);
}


int p2p_interface::start_torrent(const char *torrent_file, const char *dir, int min_port, int max_port) {
    start_torrent_(torrent_file, dir, min_port, max_port, 0, 0, 0);
}

int p2p_interface::start_torrent(const char *torrent_file, const char *dir, int demand, int upload_limit, int download_limit) {
    start_torrent_(torrent_file, dir, -1, -1, demand, upload_limit, download_limit);
}
/*
int add_hashes_db() {
	leveldb::DB* db;
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
	if (status.ok()) {
		leveldb::Slice key = "T1";
		leveldb::Slice value = "H1";
		db->Put(leveldb::WriteOptions(), key, value);

		std::string value_read;
		db->Get(leveldb::ReadOptions(), key, &value_read);
		std::cout << value_read << std::endl;
		
		return 1;
	}
	if (!status.ok()) std::cerr << status.ToString() << std::endl;
	return 0;
}
*/



int p2p_interface::start_torrent_(const char *torrent_file, const char *dir, int min_port, int max_port, int demand, int upload_limit, int download_limit) {
    if (min_port == -1) min_port = DEFAULT_MIN_PORT;
    if (max_port == -1) max_port = DEFAULT_MAX_PORT;
    	
    P2P_OUT << "start" 
	    << P2P_ENDL;
   
	//add_hashes_db(); 
    int nice = getpriority (PRIO_PROCESS, 0);
    P2P_OUT << "nice=" << nice
	    << P2P_ENDL;

    P2P_OUT << "min_port=" << min_port 
	    << ", max_port=" << max_port << P2P_ENDL;
    ses.listen_on(std::make_pair(min_port, max_port));
    add_torrent_params p;
    srand((unsigned)time(0)); 
    
    on_demand = demand; 

    p.save_path = dir;
    p.ti = new torrent_info(torrent_file);
    
    
    //torrent_info const& ti2 = h.get_torrent_info
    num_pieces = p.ti->num_pieces(); 
    piece_length = p.ti->piece_length();
    total_size = p.ti->total_size();
    if (p.ti->priv()){ P2P_OUT << "torrent_type=private" << P2P_ENDL;   }
    else{ P2P_OUT << "torrent_type=public" << P2P_ENDL; }
    P2P_OUT << "num_pieces=" << num_pieces << P2P_ENDL;
    P2P_OUT << "piece_length=" << piece_length << P2P_ENDL;
    P2P_OUT << "total_size=" << total_size << P2P_ENDL;

    //new vars: 
    blocks_per_piece = piece_length/BT_BLOCK_SIZE;
    P2P_OUT << "blocks_per_piece=" << blocks_per_piece << P2P_ENDL;

    // TODO: convert these into bitmaps at some point
    priority_request_time = NULL;
    current_piece_priority = NULL;
    piece_on_disk = NULL;
    read_status = NULL;
    if (num_pieces > 0) { 
	last_piece_size = p.ti->piece_size(num_pieces-1);	
	priority_request_time = new double[num_pieces];
	current_piece_priority = new short[num_pieces];
	piece_on_disk = new bool[num_pieces];	
        read_status = new char[num_pieces*blocks_per_piece];
	for (int i=0; i<num_pieces; i++) {
	    priority_request_time[i] = 0;
	    current_piece_priority[i] = 1;
	    piece_on_disk[i] = 0;
	    for(int j=0; j<blocks_per_piece; j++){
		read_status[i*blocks_per_piece+j] = IDLE;
	    }
	}
    }
   
    
    //if the file is indexed in 16KB blocks, what's the last block index: 

    last_block_size = last_piece_size - (int)(last_piece_size/BT_BLOCK_SIZE)*BT_BLOCK_SIZE;
    if (last_block_size == 0) {
      last_block_size = BT_BLOCK_SIZE;
    } 

    last_block_index = (num_pieces-1)*blocks_per_piece + last_piece_size/BT_BLOCK_SIZE - 1;
    if (last_block_size != BT_BLOCK_SIZE) {
      last_block_index++; 
    } 
    
    //p.storage_mode = compact_mode ? storage_mode_compact : storage_mode_sparse;
    p.paused = true;
    p.duplicate_is_error = false;
    p.auto_managed = true;
    
    h = ses.add_torrent(p);
    //    session_settings settings = ses.settings();
    session_settings settings = high_performance_seed();
    settings.ignore_limits_on_local_network = true;
    settings.allow_multiple_connections_per_ip = true;
    //settings.seed_time_limit = (7 * 24 * 60 * 60); // 7 days
    ses.set_settings(settings);	

    //    h.set_max_connections(50);
    //   h.set_max_uploads(8); // to ensure we don't have a full mesh

    // set upload and download limits
    if ( upload_limit > 0 ){ 
	P2P_OUT << "bytes/sec input upload_limit=" << upload_limit 
		<< "bits/sec input upload_limit="  << (upload_limit << 3)
		<< P2P_ENDL;
    }
    else{
	P2P_OUT << "input upload_rate=UNLIMITED" << P2P_ENDL;
    }
    if ( download_limit > 0 ) {
	P2P_OUT << "bytes/sec input download_limit=" << download_limit 
		<< "bits/sec input download_limit="  << (download_limit << 3)
		<< P2P_ENDL;
    }
    else{
	P2P_OUT << "input download_rate=UNLIMITED" << P2P_ENDL;
    }
    h.set_download_limit(download_limit);
    h.set_upload_limit(upload_limit);

    settings = ses.settings();    
    if ( h.upload_limit() > 0 ) {
	P2P_OUT << "bytes/sec current upload_limit=" << h.upload_limit() 
		<< "bits/sec current upload_limit="  << (h.upload_limit() << 3)
		<< P2P_ENDL;
    }
    else{
	P2P_OUT << "current upload_rate=UNLIMITED" << P2P_ENDL;
    }

    if ( h.download_limit() > 0 ) {
	P2P_OUT << "bytes/sec current download_limit=" << h.download_limit() 
		<< "bits/sec current download_limit="  << (h.download_limit() << 3)
		<< P2P_ENDL;
    }
    else {
	P2P_OUT << "current download_rate=UNLIMITED" << P2P_ENDL;
    }
    P2P_OUT << "send_buffer_watermark=" << settings.send_buffer_watermark << P2P_ENDL;
    P2P_OUT << "send_socket_buffer_size=" << settings.send_socket_buffer_size << P2P_ENDL;
    P2P_OUT << "cache_size=" << settings.cache_size << P2P_ENDL;
    P2P_OUT << "allow_multiple_connections_per_ip=" << settings.allow_multiple_connections_per_ip << P2P_ENDL;
    P2P_OUT << "ignore_limits_on_local_network=" << settings.ignore_limits_on_local_network << P2P_ENDL;
    P2P_OUT << "seed_time_limit=" << settings.seed_time_limit << P2P_ENDL;
    P2P_OUT << "auto_upload_slots=" << settings.auto_upload_slots << P2P_ENDL;
    P2P_OUT << "max_allowed_in_request_queue=" << settings.max_allowed_in_request_queue << P2P_ENDL;
    P2P_OUT << "max_out_request_queue=" << settings.max_out_request_queue << P2P_ENDL;
    P2P_OUT << "request_queue_time=" << settings.request_queue_time  << P2P_ENDL;
    P2P_OUT << "max_failcount=" << settings.max_failcount << P2P_ENDL;
    

#ifndef TORRENT_DISABLE_RESOLVE_COUNTRIES
    //h.resolve_countries(true);
#endif
    
    //sleep(5);
        
    h.register_p2pinterface_callback((void*)this, p2p_interface::piece_downloaded_wrapper);
    P2P_OUT << "torrent added" << P2P_ENDL;

    // PUSH ALL BLOCKS TO PRIORITY TWO (TO CIRCUMVENT THROTTLING WHILE LEAVING OVERHEADS IN PLACE)
    //init_priorities(2);

    while(1) {
	torrent_status tstat = h.status();
	if (tstat.state == torrent_status::downloading ||
	    tstat.state == torrent_status::finished ||
	    tstat.state == torrent_status::seeding) {
	  torrent_started = true;

	  //If any peers were specified inside add_peers_file, then 
	  //connect to these peers directly: 
	  if (!peers.empty()) {
	    connect_to_peers(); 
	  } 
	  	
	  P2P_OUT << "done" 
		  << P2P_ENDL;
	  return 0; 
	} 
	sleep(1);
    } 
    // wait for the user to end
    /*
      char a;
      std::cin.unsetf(std::ios_base::skipws);
      std::cin >> a;
    */
}


void p2p_interface::add_dht_node(const char *seed_host, int port) {
    const std::pair<std::string, int>  node(seed_host, port); 
    ses.add_dht_node(node);
} 


void p2p_interface::add_peer(const char *host, int port) {
  char buf[50]; 
  sprintf(buf, "%d", port); 
  add_peer(host,buf);
} 

void p2p_interface::add_peer(const char *host, const char *port) {
  char *hbuf = strdup(host);
  char *pbuf = strdup(port);
  peers[hbuf] = pbuf; 
} 

  //API function add_peer_file adds a file in the format:
  //<host:port> 
  //<host:port>
  //...
  //if port is omitted, the default 6881 is used 
int p2p_interface::add_peer_file(const char *filename) {
    using namespace std;

    //struct timeval tv;
    ifstream ifs(filename);
    string line;
    string host; 
    string port; 
    
    while(getline(ifs,line)) {
      if (line.size() < 1) {
	continue;
      } 

      //only interested in the string preceding the comment:"#"
      int comment = line.find('#');
      if (comment == 0) {
	continue; 
      } 
      if (comment > 0) {
	line = line.substr(0, comment);
      } 
      
      //get the first word on the line only
      stringstream st(line);
      if (st.good()) {
	st >> line; 
      }
      else {
	continue; 
      } 

      //see if the word contains a ":" , then it's a host:port, otherwise
      //the whole thing is a host name: 
      int pos = line.find(':'); 
      if (pos == string::npos) {
	host = line; 
	port = "6881"; 
	P2P_OUT << "adding,\thost=" << host << ", port=" << port << P2P_ENDL;
    	add_peer(host.c_str(),port.c_str()); 
      } 
      else if (pos < 1 || pos == line.size()-1) {
	continue; 
      } 
      else {
	host  = line.substr(0,pos);
	port = line.substr(pos+1);
	P2P_OUT << "adding,\thost=" << host << ", port=" << port << P2P_ENDL;
	add_peer(host.c_str(),port.c_str()); 
      } 
    }

    //if the torrent is started, and we have some peers, connect to them: 
    if (torrent_started && !peers.empty()) {
	P2P_OUT << "connect_to_peers start" << P2P_ENDL;
	connect_to_peers(); 
	P2P_OUT << "connect_to_peers end" << P2P_ENDL;
    }

    P2P_OUT << "done" 
	    << P2P_ENDL;

    return 0;
} 

void p2p_interface::connect_to_peers() { 
  //TO avoid delays on startup we now connect directly to peers
  //that have been specified: 
  std::map<const char *, const char *>::iterator iter; 
  for(iter=peers.begin(); iter!=peers.end(); iter++) {
    
    boost::asio::io_service io_service;
    
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), iter->first, iter->second);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;
    
    if (endpoint_iterator != end) { 
      h.connect_peer(*endpoint_iterator);
    }
  } 
}



// priority reported is actual if first raise or 10 + actual if subsequent raise
// if more than 2 raises were possible, implementation should be changed so
// nth raise would be reported at priority ((n-1)*10 + actual)
// priority_request_time is always the time of the FIRST priority raise
void p2p_interface::set_piece_priority(int piece_index, int priority) {

    if ( piece_index < 0 || piece_index >= num_pieces ) return; 
    P2P_TORRENT_LOCK(piece_index);
    if (t) {
        if (!t->have_piece(piece_index) && !piece_on_disk[piece_index]) {
	    if ( current_piece_priority[piece_index] < priority ){
		if (priority_request_time[piece_index] == 0 ) {
		    struct timeval tv;
		    gettimeofday(&tv,NULL);
		    current_piece_priority[piece_index] = priority;
		    priority_request_time[piece_index] = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
		}
		else{
		    current_piece_priority[piece_index] = 10 + priority;
		}
		h.piece_priority(piece_index, priority);     
		P2P_OUT << "set"
			<< ", piece=" << piece_index 
			<< ",\tpriority=" << priority 
			<< P2P_ENDL;
	    }
	}
    }
    else{
	P2P_OUT << "failed to get torrent lock" 	    
		<< ", piece=" << piece_index 
		<< ",\tpriority=" << priority 
		<< P2P_ENDL;
    }
}

  //API function add_profile
  //takes a pointer to a profile file that contains 
  //two words per line: <16KB block index> <first access timestamp> 
  //if port is omitted, the default 6881 is used 
int p2p_interface::add_profile(const char *filename, int priority) {
    using namespace std;
    
    ifstream ifs(filename);
    string line;
    string block_index_str;
    string rank_str;
    string mean_t_str; 
    string freq_str; 
    string freq_stddev_str; 
    string min_t_str; 
    
    int first_access = -1; 
    
    pending_profile.clear(); 
    active_profile.clear(); 
    
    std::map<int,int> pieces; 

    while(getline(ifs,line)) {
	if (line.size() < 1) {
	    P2P_OUT << "ERROR=line size too small" 
		    << P2P_ENDL; 
	    continue;
	} 
	
	//get the first two strings from the line: 
	stringstream st(line);
	if(!st.good() ||
	   !(st >> block_index_str) || 
	   !(st >> rank_str) ||
	   !(st >> mean_t_str) || 	  
	   !(st >> freq_str) || 	  
	   !(st >> freq_stddev_str) || 	  
	   !(st >> min_t_str)) { 
	    P2P_OUT << "ERROR=stringstream" 
		    << P2P_ENDL; 
	    continue; 
	} 

	int block_index = atoi(block_index_str.c_str());
	block_index = block_index >> (BT_BLOCK_BITS - 14); // HACK ALERT! ASSUMES PROFILES ARE IN 16KB BLOCKS

	double min_t = (double)(atof(min_t_str.c_str()));
	double mean_t = (double)(atof(mean_t_str.c_str()));
	double freq = (double)(atof(freq_str.c_str()));
	
	if (block_index < 0){  
		P2P_OUT << "ERROR=block_index"
			<< ", block_index=" << block_index 
			<< P2P_ENDL;
	    pending_profile.clear(); 
	    active_profile.clear(); 
	    return -1; 
	} 

	if ( block_index > last_block_index) {
	    P2P_OUT << "ERROR=out of range" 
		    << ", block_index=" << block_index 
		    << P2P_ENDL;
	    continue;
	}
	
	int piece_index = block_index/blocks_per_piece;
	
	if (pieces.find(piece_index) == pieces.end()) { 
	    pieces[piece_index] = 1; 
	    pending_profile.push_back(profile_entry(piece_index,min_t,mean_t,freq));
	    P2P_OUT << "add_to_profile" 
		    << ", piece=" << piece_index 
		    << ", min_access_time=" << min_t 
		    << ", mean_access_time=" << mean_t 
		    << ", frequency=" << freq 
		    << P2P_ENDL;
	}
    }
    return apply_profile(bt_profile_window, priority); 
}
	

void p2p_interface::check_profile(int piece_index){
    if (priority_request_time[piece_index] > 0){
	struct timeval tv;
	gettimeofday(&tv, NULL); 
	double curtime = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);

	P2P_PTHREAD_MUTEX_LOCK(&priority_lock,piece_index);
	for(profile::iterator pi = active_profile.begin(); pi != active_profile.end(); pi++)
	    if (pi->piece_index == piece_index){
	      
	        active_profile.erase(pi);
		int profile_deficit = bt_profile_window - active_profile.size();
		if (profile_deficit > pending_profile.size()) 
		    profile_deficit = pending_profile.size();
		if ( profile_deficit > 0 ){
		    apply_profile(profile_deficit, PROFILE_CLOSE_PRIORITY);
		}
		break;
	    }	
	pthread_mutex_unlock(&priority_lock);
    }
    else{
	//IF IN PROFILE AND DOWNLOADED BEFORE DEMANDED, THEN REMOVE FROM PENDING PRIORITY REQUESTS
	P2P_PTHREAD_MUTEX_LOCK(&priority_lock,piece_index);
	for(profile::iterator pi = pending_profile.begin(); pi != pending_profile.end(); pi++)
	    if (pi->piece_index == piece_index){
		pending_profile.erase(pi);
		break;
	    }	
	pthread_mutex_unlock(&priority_lock);
    }
}



//This function applies the previous read in profile, by changing the 
//priority vector:
int p2p_interface::apply_profile(int width, int priority) {

    int count=0;

    while ( !pending_profile.empty() && count < width){
     	if ( pending_profile.size() < random_diversity_window ) random_diversity_window = pending_profile.size();
	int rank = rand() % random_diversity_window;	
	profile::iterator pi = pending_profile.begin();

	pi +=  rank;	
	if ( pi == pending_profile.end() ){
	    P2P_OUT << "ERROR=iterator incremented too far" 
		    << P2P_ENDL;
	    continue;
	}
	profile_entry pe = *pi; 
	pending_profile.erase(pi);

	if (pe.piece_index < 0 || 
	    pe.piece_index >= num_pieces) {
	    P2P_OUT << "ERROR=priority index out of range" 
		  << pe.piece_index 
		    << P2P_ENDL; 
	    continue; 
	} 
	  	
	P2P_OUT << "chose" 
		<< ", piece=" << pe.piece_index 
		<< ",\trank=" << rank 
		<< P2P_ENDL;  

	if (current_piece_priority[pe.piece_index] < priority ) {
	    set_piece_priority(pe.piece_index, priority);
	    active_profile.push_back(pe);
	    count++; 
#ifdef VMTORRENT_PUSH
	    if ( p2p_push ){
		long block_index = pe.piece_index*blocks_per_piece; 
		pthread_t thread;
		P2P_OUT << "spawning p2p_push callback"
			<< ",\tpiece=" << pe.piece_index 
			<< P2P_ENDL;
		pthread_create( &thread, NULL, p2p_push, (void*) (block_index));
		P2P_OUT << "spawned p2p_push callback" 
			<< ",\tpiece=" << pe.piece_index 
			<< P2P_ENDL;
	    }
#endif
	}
    }
    return 0; 
} 


int p2p_interface::init_priorities(int priority) {
    P2P_OUT << "start" 
	    << P2P_ENDL;   

    P2P_TORRENT_LOCK("");
    if (t) {
	std::vector<int> cur_priorities = h.piece_priorities();
	for(int i=0; i<cur_priorities.size(); i++){
	    cur_priorities[i] = priority;
	    P2P_OUT << "new priority"
		    << ", piece=" << i 
		    << ", priority=" << cur_priorities[i] 
		    << P2P_ENDL;
	}
	h.prioritize_pieces(cur_priorities);
    }
    P2P_OUT << "done" 
	    << P2P_ENDL;
    return 0; 
} 



torrent_info const& p2p_interface::get_torrent_info() {
    P2P_TORRENT_LOCK("");
    if (t) {
	return h.get_torrent_info();
    } 
    else {
	exit(1); 
    } 
}


    //Thread Protected: 
    //This function is called by a thread that wants to read in a given piece
    //Into a pre-allocated buffer. This call will block until the data is
    //Read in.
    //Returns -1 on error, or an integer <= bufsize with the number of bytes_written
    //the new get_block function handles the case where the underlying p2p file
    //is broken into > 16KB chunks. In that case BT_BLOCK_SIZE is still 16KB. 
    //p2p_interface::piece_length refers to the size of the p2p piece. 
    //where a piece may be compose of 2^i 16KB blocks. 
int p2p_interface::get_block(int block_index, char**pbuffer, int bufsize) {
    
    int written = -1; 
    int piece_index = block_index/blocks_per_piece; 
    int piece_offset = (block_index - (piece_index*blocks_per_piece))*BT_BLOCK_SIZE;
    
    if (piece_index > num_pieces-1 ||
	piece_index < 0 ||
	block_index > last_block_index ||
	(block_index < last_block_index && bufsize < BT_BLOCK_SIZE)) {
	P2P_OUT << "ERROR=index_check" 	
		<< ",piece=" << piece_index	
		<< ",block=" << block_index 
		<< P2P_ENDL;
	return 0; 
    }
    pthread_t tid = pthread_self();
    block_request *br = new block_request(block_index, tid, pbuffer, bufsize);
    
    P2P_PTHREAD_MUTEX_LOCK(&req_lock,piece_index); 
    
    request_entry re = requests.insert(br); 
    
    if (re.rp == NULL) {
	//ALERT 
	pthread_mutex_unlock(&req_lock);
	P2P_OUT << "ERROR=re.rp is NULL" 
		<< ",piece=" << piece_index 
		<< ",block=" << block_index 
		<< P2P_ENDL;
	delete br;
	return -1; 
    } 
    
    //issues  async call to read the block if it's already on disk
    int ret = read_from_piece(piece_index, piece_offset, block_index);
    
    if(ret<0) {
	//error
	pthread_mutex_unlock(&req_lock);
	P2P_OUT << "ERROR=read_from_piece failed"
		<< ",piece=" << piece_index 
		<< ",block=" << block_index 
		<< P2P_ENDL;
	return -1; 
    }
    
    while(!re.rp->completed) {
	pthread_cond_wait(&(re.rp->cond), &req_lock); 
    }
    
    written = br->bytes_written;
       
    //finally remove request
    requests.remove(br); 
    read_status[block_index] = IDLE;

    pthread_mutex_unlock(&req_lock);
    delete br;
    return written;
}


int p2p_interface::read_from_piece(int piece_index, int piece_offset, int block_index) {

    int retval = -2;
    
    P2P_TORRENT_LOCK(piece_index);
    if (t) {
	if(t->have_piece(piece_index)){
	    if(read_status[block_index] != READ_IN_PROG){

	        peer_request r = {piece_index, piece_offset, BT_BLOCK_SIZE};
		if (block_index == last_block_index) {
		    r.length = last_block_size;
		}
		
		t->filesystem().async_read(r, bind(&p2p_interface::on_disk_read_complete,this, _1, _2, r));
		read_status[block_index] = READ_IN_PROG;
		retval = 0;
	    }
	    else{
		retval = 1;
	    }
	}
	else{
	    read_status[block_index] = WAITING_TO_ISSUE_READ;
	    retval = 2;
	}
    }
    else{
	read_status[block_index] = WAITING_TO_ISSUE_READ;
	retval = -1;
    }
    
    return retval; 
    // -2 should never happen, -1 if couldn't get lock
    // 1 if async_read called, 2 if async_read already called
}



int piece_num = 0;
void p2p_interface::piece_downloaded(int piece_index) {

    P2P_OUT << "start" 
	    << ",piece=" << piece_index 
	    << P2P_ENDL;

    P2P_PTHREAD_MUTEX_LOCK(&req_lock,piece_index); 
    piece_on_disk[piece_index] = 1;
    piece_num++;
    pthread_mutex_unlock(&req_lock);


    for(int i=0; i<blocks_per_piece; i++) {
	int block_index = piece_index*blocks_per_piece + i; 

	P2P_PTHREAD_MUTEX_LOCK(&req_lock,piece_index); 
	request_entry re = requests.find_by_block_index(block_index);
	
	if (re.rp != NULL) {
	    P2P_TORRENT_LOCK(piece_index);
	    if (t) {
		if (read_status[block_index] == WAITING_TO_ISSUE_READ ){
		    peer_request r = {piece_index, i*BT_BLOCK_SIZE, BT_BLOCK_SIZE};
		    if (block_index == last_block_index) {
			r.length = last_block_size;
		    } 
		    
		    t->filesystem().async_read(r, bind(&p2p_interface::on_disk_read_complete,this, _1, _2, r));
		    read_status[block_index] = READ_IN_PROG;
		}
	    }
 	}
	pthread_mutex_unlock(&req_lock);
    }

    P2P_OUT << "done"
	    << ", piece=" << piece_index 
	    << P2P_ENDL;
}


void p2p_interface::on_disk_read_complete(int ret, disk_io_job const& j, peer_request r) {
 
    //using disk buffer_holder will deallocate j.buffer
    //when disk_buffer_holder goes out of scope
    disk_buffer_holder buffer(*(ses.get_session_imp()), j.buffer);
    
    request_entry re(NULL, NULL); 
    
    int block_index = r.piece*blocks_per_piece + r.start/BT_BLOCK_SIZE; 

    P2P_PTHREAD_MUTEX_LOCK(&req_lock,r.piece);
    if (ret != r.length) {
	re = requests.find_by_block_index(block_index);
    }
    else {
	//convert request format: 
	peer_request converted_req = {block_index, 0, r.length};
	re = requests.copy_buffer(converted_req, j); 
    }
    
    if (re.rp !=NULL) {
	re.rp->completed = true;
	pthread_cond_broadcast(&(re.rp->cond));
    }

    pthread_mutex_unlock(&req_lock); 
}


