#include "p2p_leveldb.hpp"
//using namespace std;

int main(int argc, char** argv)
{
std::string a = "heeey";
insert_hash("hi", &a);
//leveldb_inst * temp;

std:: string temp = get_torrent_path("hi");
    std::cout << temp<<std::endl;
bool b = update_keys("hi","hii");

std::cout<<b;

if (b){
 temp = get_torrent_path("hii");
    std::cout << temp<<std::endl;
}


}

