#include "connected_client.h"

void Connected_client::init(string username, int sockfd, int num_connections, int port, int header_size, int max_payload)
{
	this->username = username;
	this->sockfd = sockfd;
	this->num_connections = num_connections;
	this->max_connections = 2;
	this->user_files_pointer = &user_files;
	this->user_files_mutex_pointer = &user_files_mutex;

	com.Init(port, header_size, max_payload);
}

void Connected_client::init(string username, int sockfd, int num_connections, int port, int header_size, int max_payload, vector<File_server> *user_files_pointer, pthread_mutex_t *user_files_mutex_pointer)
{
	this->username = username;
	this->sockfd = sockfd;
	this->num_connections = num_connections;
	this->max_connections = 2;
	this->user_files_pointer = user_files_pointer;
	this->user_files_mutex_pointer = user_files_mutex_pointer;

	com.Init(port, header_size, max_payload);
}

string *Connected_client::get_username() {return &username;}

int *Connected_client::get_sockfd() {return &sockfd;}

int Connected_client::get_num_connections() {return num_connections;}

pthread_t Connected_client::get_thread() {return thread;}

void Connected_client::set_thread(pthread_t thread) {this->thread = thread;}

vector<File_server> *Connected_client::get_user_files()
{
	return user_files_pointer;
}

pthread_mutex_t *Connected_client::get_user_files_mutex()
{
	return user_files_mutex_pointer;
}

int Connected_client::new_connection()
{
	if(num_connections+1 > max_connections)
		return -1;
	else
	{
		num_connections++;
		return num_connections;
	}
}

void Connected_client::remove_connection() {num_connections--;}
