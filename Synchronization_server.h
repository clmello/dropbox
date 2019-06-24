#ifndef SYNCHRONIZATION_SERVER_H
#define SYNCHRONIZATION_SERVER_H
#include "connected_client.h"
#include "Communication_server.h"
#include "stdint.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <netdb.h>
#include <fcntl.h>

using namespace std;

class Synchronization_server
{
	public:
		void Init(int port);

	protected:

	private:
		int client_accept_sockfd;
		int backup_accept_sockfd;
		int chk_accept_sockfd;
		int port;
		int backup_port;
		int chk_port;
		int header_size;
		int max_payload;
		int packet_size;
		char* buffer;
		size_t buffer_address;
		struct packet* header;
		size_t header_address;
		struct file{time_t mtime; string name;};
		vector<file> watched_files;
		vector<Connected_client> connected_clients;
		vector<int*> threads_finished_address;
		//vector<int> backup_sockets;
		//vector<int> backup_sockets;
		//vector<pthread_mutex_t> backup_mutexes;
		//pthread_mutex_t r_w_backups_mutex;
		//vector<int> r_w_backups;
		vector<int> chk_sockets;

		void accept_connections();
		void close_server();

		void signal_alive();

		void check_finished_threads();

		packet* receive_payload(int sockfd);// Receives the _payload of the packet from the client and returns a packet struct containing the _payload
		packet* receive_header(int sockfd);	// Receives the header of the packet from the client and returns a packet struct containing the header
};

#endif // SYNCHRONIZATION_SERVER_H
