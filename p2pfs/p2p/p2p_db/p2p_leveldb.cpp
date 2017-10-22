#include "p2p_leveldb.hpp"

bool insert_hash(std::string hash, std::string path, int offset, int bytes ) {
    
    std::ostringstream c1,c2; 
    c1 << offset;
    c2 << bytes;
    std::string value = path+":"+c1.str()+":"+c2.str()+":";
    
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
    db->Put(writeOptions, hash, value);
    
    delete db;

}

leveldb_inst * get_torrent_path(std::string hash) {
    
    std::string value;

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, "./torrent_db", &db);
    leveldb::Status s = db->Get(leveldb::ReadOptions(), hash, &value);
    
    if (s.ok()) {
       std::list<std::string> temp;
       std::string delimiter = ":";

       size_t pos = 0;
       std::string token;
       while ((pos = value.find(delimiter)) != std::string::npos) {
          token = value.substr(0, pos);
          temp.push_back(token);
          value.erase(0, pos + delimiter.length());
        }
       
       if (temp.size() == 3 ){
           leveldb_inst * inst = new leveldb_inst(); 
       	   inst->file_path = temp.front();
           temp.pop_front();
           std::stringstream convert1(temp.front());
           convert1 >> inst->offset;
           temp.pop_front();
           std::stringstream convert2(temp.front());
           convert2 >> inst->tot_bytes;
           temp.pop_front();
           delete db;
           return inst;
       }
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
