#include "p2p_leveldb.hpp"

bool insert_hash(std::string hash, std::string *block ) {
    
    /*std::ostringstream c1,c2; 
    c1 << offset;
    c2 << bytes;
    std::string value = path+":"+c1.str()+":"+c2.str()+":";
    */
    //std::cout<<value;
    
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, "./torrent_db", &db);

    if (false == status.ok())
    {
        std::cerr << "Unable to open/create test database './torrent_db'" << std::endl;
        std::cerr << status.ToString() << std::endl;
        return -1;
    }
    
    leveldb::WriteOptions writeOptions;
    db->Put(writeOptions, hash, *block);
    
    delete db;

}

std::string get_torrent_path(std::string hash) {
    
    std::string block;

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, "./torrent_db", &db);
    leveldb::Status s = db->Get(leveldb::ReadOptions(), hash, &block);
    
    if (s.ok()) {
           delete db;
           return block;
    }
    delete db;
    return NULL;

    
}

/*
int main(){

insert_hash("hi", "bye",1, 3);
leveldb_inst * temp;
temp = get_torrent_path("hi");
if (temp) {
    std::cout << temp->file_path << " " << temp->offset<<" "<<temp->tot_bytes<<std::endl;
    }
}
*/
