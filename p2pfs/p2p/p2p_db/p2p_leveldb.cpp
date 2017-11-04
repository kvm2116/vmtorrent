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
        return false;
    }
    
    leveldb::WriteOptions writeOptions;
    db->Put(writeOptions, hash, *block);
    
    delete db;
    return true;

}

bool update_keys(std::string key1, std::string key2) {

    std::string value;

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, "./torrent_db", &db);

    leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
    if (s.ok()) {
        leveldb::WriteBatch batch;
  	batch.Delete(key1);
  	batch.Put("hii", value);
  	s = db->Write(leveldb::WriteOptions(), &batch);
        delete db;
	return true;
    }
    
    delete db;
    return false;
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


void print_all()
{
    // Set up database connection information and open database
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, "./torrent_db", &db);

    if (false == status.ok())
    {
        std::cerr << "Unable to open/create test database './torrent_db'" << std::endl;
        std::cerr << status.ToString() << std::endl;
        return ;
    }
    
    // Iterate over each item in the database and print them
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        std::cout << it->key().ToString() << " : " << it->value().ToString() << std::endl;
    }
    
    if (false == it->status().ok())
    {
        std::cerr << "An error was found during the scan" << std::endl;
        std::cerr << it->status().ToString() << std::endl; 
    }
    
    delete it;
    
    // Close the database
    delete db;
}

