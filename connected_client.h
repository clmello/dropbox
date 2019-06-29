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
		void init(string username, int sockfd, int num_connections, int port, int header_size, int max_payload);
		// If this is the second client from the same user, the vector of files and its mutex must be the same
		void init(string username, int sockfd, int num_connections, int port, int header_size, int max_payload, vector<File_server> *user_files_pointer, pthread_mutex_t *user_files_mutex_pointer);

		string *get_username();
		int *get_sockfd();
		int get_num_connections();
		pthread_t get_thread();
		vector<File_server> *get_user_files();
		int get_user_files_size();
		pthread_mutex_t *get_user_files_mutex();

		// This method MUST BE CALLED after the creation of an object
		void set_thread(pthread_t thread);

		// Returns num_connections+1 if num_connections+1 < max_connections, -1 otherwise
		int new_connection();
		void remove_connection();

		Communication_server com;

	private:
		string username;
		int sockfd;
		int num_connections;
		int max_connections;
		string mtimes_file_path;

		pthread_t thread;

		vector<File_server> user_files;
		vector<File_server> *user_files_pointer;
		pthread_mutex_t user_files_mutex;
		pthread_mutex_t *user_files_mutex_pointer;
		
		void files_from_disk();

};

#endif // CONNECTED_CLIENT_H
