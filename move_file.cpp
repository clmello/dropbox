//#include <string>
#include <fstream>

int copy_file(std::string original_path, std::string copy_path)
{
    std::ifstream  src(original_path.c_str(), std::ios::binary);
    std::ofstream  dst(copy_path.c_str(),   std::ios::binary);

    dst << src.rdbuf();
}

int main()
{
    std::string original_path = "/home/alack/sync_dir_bla/teste.txt";
    std::string copy_path = "/home/alack/sync_dir_bla/teste2.txt";
    copy_file(original_path, copy_path);
    return 0;
}
