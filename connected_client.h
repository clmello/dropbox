#ifndef CONNECTED_CLIENT_H
#define CONNECTED_CLIENT_H

#include "Communication_server.h"

#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>

using namespace std;

class Connected_client{

	public:
		Connected_client(string username, int sockfd, int num_connections, int port, int header_size, int max_payload);

		string *get_username();
		int *get_sockfd();
		int get_num_connections();
		pthread_t get_thread();
		//bool *get_thread_finished();
		//bool is_finished();
		
		// This method MUST BE CALLED after the creation of an object
		void set_thread(pthread_t thread);

		// Returns num_connections+1 if num_connections+1 < max_connections, -1 otherwise
		int new_connection();
		
		Communication_server com;

	private:
		string username;
		int sockfd;
		int num_connections;
		int max_connections;
		
		pthread_t thread;
		//bool thread_finished;

};

#endif // CONNECTED_CLIENT_H
