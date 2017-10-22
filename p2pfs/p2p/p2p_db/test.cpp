#include "p2p_leveldb.hpp"
//using namespace std;

int main(int argc, char** argv)
{
insert_hash("hi", "bye",1, 3);
leveldb_inst * temp;
temp = get_torrent_path("hi");
if (temp) {
    std::cout << temp->file_path << " " << temp->offset<<" "<<temp->tot_bytes<<std::endl;
    }
}

