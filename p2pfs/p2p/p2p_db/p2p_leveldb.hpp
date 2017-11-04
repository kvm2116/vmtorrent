#include <iostream>
#include <sstream>
#include <string>
#include <list>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"
void print_all();
/*
typedef struct leveldb_inst_ {
        std::string file_path;
        int offset;
        int tot_bytes;
    }leveldb_inst;
*/

bool insert_hash(std::string hash, std::string *block);
std::string get_torrent_path(std::string hash);
bool update_keys(std::string key1, std::string key2);
