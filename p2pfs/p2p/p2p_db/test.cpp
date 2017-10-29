#include "p2p_leveldb.hpp"
//using namespace std;

int main(int argc, char** argv)
{
std::string a = "heeey";
insert_hash("hey", &a);
//leveldb_inst * temp;

std:: string temp = get_torrent_path("hi");
    std::cout << temp<<std::endl;

 temp = get_torrent_path("hey");
    std::cout << temp<<std::endl;
}

