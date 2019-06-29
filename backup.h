#ifndef BACKUP_H
#define BACKUP_H
#include "Synchronization_server.h"
#include <iostream>
#include <string.h>

using namespace std;

struct bkp_args
{
	void* obj = NULL;
    int *main_check_sockfd = NULL;
    int *server_died = NULL;
};

class Backup
{
    public:
        Backup(string main_ip, int main_port);
    private:
        string main_ip;
        int main_port;
        int main_sockfd;
        int chk_sockfd;
		int header_size;
		int max_payload;
		int packet_size;
		char* buffer;
		size_t buffer_address;
		struct packet* header;
		size_t header_address;
        struct file{time_t mtime; time_t local_mtime; std::string name;};
        std::vector<file> watched_files;
        string dir;
        pthread_t chk_thread;
		bool connected;

		pthread_t connect_backups_thread;

		std::vector<int> backup_sockets;
		std::vector<std::string> backup_ips;

        static void *check_server_helper(void *void_args);
        void check_server(int* main_check_sockfd, int *server_died);
		static void *connect_backups_helper(void *void_args);
		void connect_backup(int* main_check_sockfd, int *server_died);
        int connect_backup_to_main();
        int connect_chk_server();
        void close_backup(int main_check_sockfd);
        void receive_commands(int sockfd, int *server_died);
        string create_user_folder(string username);
        int receive_int(int sockfd);
		packet* receive_payload(int sockfd, int timeout_sec);
		packet* receive_header(int sockfd, int timeout_sec);
		void receive_file(int sockfd, string path);
		void receive_server_files(int sockfd);
		void mtime_to_file(string path, time_t mtime, string username);
		void delete_mtime_from_file(string path, string username);
};


#endif // BACKUP_H
