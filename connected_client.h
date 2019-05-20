#ifndef CONNECTED_CLIENT_H
#define CONNECTED_CLIENT_H

#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>

using namespace std;

class Connected_client{

	public:
		Connected_client(pthread_t thread, string username, int sockfd);

		string get_username();
		int get_sockfd();
		int get_num_connections();
		pthread_t get_thread();

		// Returns 0 if num_connections+1 < max_connections, -1 otherwise
		int new_connection();

	private:
		string username;
		int sockfd;
		int num_connections;
		int max_connections;
		pthread_t thread;

};

#endif // CONNECTED_CLIENT_H
