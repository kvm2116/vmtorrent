#include <iostream>
#include <sstream>
#include <string>
#include <list>

#include "leveldb/db.h"

typedef struct leveldb_inst_ {
        std::string file_path;
        int offset;
        int tot_bytes;
    }leveldb_inst;

bool insert_hash(std::string hash, std::string path, int offset, int bytes );
leveldb_inst * get_torrent_path(std::string hash);
