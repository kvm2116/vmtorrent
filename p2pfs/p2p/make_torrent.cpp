/*

Copyright (c) 2006, Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include <iostream>
#include <fstream>
#include <iterator>
#include <iomanip>

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/file_pool.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/bind.hpp>
#include "leveldb/db.h"
#include "leveldb/options.h"

using namespace boost::filesystem;
using namespace libtorrent;

// do not include files and folders whose
// name starts with a .
bool file_filter(boost::filesystem::path const& filename)
{
	if (filename.string().at(0) == '.') return false;
	std::cerr << filename << std::endl;
	return true;
}

void print_progress(int i, int num)
{
	std::cerr << "\r" << (i+1) << "/" << num;
}

int main(int argc, char* argv[])
{
	using namespace libtorrent;
	using namespace boost::filesystem;

	int piece_size = 16*1024; //256 * 1024;
	char const* creator_str = "libtorrent";

#ifndef BOOST_NO_EXCEPTIONS
	try
	{
#endif
	    
		file_storage fs;
		file_pool fp;
		//path full_path = complete(path(argv[3]));
		char opt; 

		path torrent_file; 
		path full_path; 
		char tracker_str[1000]; 
		char url_seed[1000]; 
		
		bool file_path_given = false; 
		bool torrent_path_given = false; 
		bool tracker_given = false; 
		bool url_seed_given = false; 
		bool priv=false;

		while((opt = getopt(argc, argv, "t:f:r:u:s:p"))!=-1) {
		    switch(opt) {
		    case 't':
			std::cout << optarg;
			torrent_file = complete(path(optarg)); 
			torrent_path_given = true; 
			break;
			
		     case 'f':
			 std::cout << optarg;
			full_path = complete(path(optarg)); 
			file_path_given = true; 
			break;
 
		    case 'r':
			 std::cout << optarg;
			strcpy(tracker_str, optarg);
			tracker_given = true; 
			break;

		    case 'u':
			strcpy(url_seed, optarg);
			url_seed_given = true; 
			break;

		    case 's':
			 std::cout <<"s: "<< optarg;
			piece_size = atoi(optarg)*1024;
			break;

		    case 'p':
			priv=true;
			break;
		    }
		}
		
		if (!torrent_path_given || !file_path_given) {

		    std::cerr << "usage: make_torrent "
			      << "-t <torrent file> "
			      << "-f <file or directory from which to create torrent> "
			      << "[-s <piece size in KB>] "
			      << "[-p] make private "
			      << "[-r <announce url>] "
			      << "[-u url-seed] " 
			      << std::endl;
		    return 0;
		} 

		add_files(fs, full_path.string(), file_filter);
		if (fs.num_files() == 0)
		{
			std::cerr << "no files specified." << std::cerr;
			return 1;
		}
		std::cout<<"Piece size is: "<<piece_size;
		create_torrent t(fs, piece_size);
		if (tracker_given) { 
		    t.add_tracker(tracker_str);
		}

		t.set_priv(priv); // apply private setting
		std::cout<<full_path.branch_path().string();
		set_piece_hashes(t, full_path.branch_path().string()
			, boost::bind(&print_progress, _1, t.num_pieces()));
		std::cerr << std::endl;
		std::cout<<std::endl<<"Creator str: "<<creator_str;
		t.set_creator(creator_str);
		if (url_seed_given) t.add_url_seed(url_seed);

		// create the torrent and print it to out
		ofstream out(torrent_file, std::ios_base::binary);
		bencode(std::ostream_iterator<char>(out), t.generate());
#ifndef BOOST_NO_EXCEPTIONS
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
#endif

	return 0;
}
