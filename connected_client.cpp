#include "connected_client.h"

Connected_client::Connected_client(pthread_t thread, string username, int sockfd)
{
	this->thread = thread;
	this->username = username;
	this->sockfd = sockfd;
	this->num_connections = 1;
}

int Connected_client::get_sockfd() {return sockfd;}

int Connected_client::get_num_connections() {return num_connections;}

int Connected_client::new_connection()
{
	if(num_connections+1 > max_connections)
		return -1;
	else
	{
		num_connections++;
		return 0;
	}
}

