#include "connected_client.h"

Connected_client::Connected_client(string username, int sockfd, int num_connections, int port, int header_size, int max_payload)
{
	this->username = username;
	this->sockfd = sockfd;
	this->num_connections = num_connections;
	this->max_connections = 2;
	
	com.Init(port, header_size, max_payload);
}

string *Connected_client::get_username() {return &username;}

int *Connected_client::get_sockfd() {return &sockfd;}

int Connected_client::get_num_connections() {return num_connections;}

pthread_t Connected_client::get_thread() {return thread;}

void Connected_client::set_thread(pthread_t thread) {this->thread = thread;}

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

