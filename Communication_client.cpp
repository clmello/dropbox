#include "Communication_client.h"

#include <iostream>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

Communication_client::Communication_client() {
	this->payload_size = 502;
	this->header_size = 10;
	this->packet_size = header_size + payload_size;
}

bool Communication_client::connect_client_server(Client client) {
	int n;
	int port = client.getPort();
	std::string hostname = client.getHostname();
	std::string username = client.getUsername();
	bool connected = false;
    struct sockaddr_in serv_addr; // server_address
    struct hostent *server = gethostbyname(hostname.c_str());

	if (server == NULL) {
		std::cerr << "ERROR, no such host\n";
        return connected;
    }

    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd == -1) {
		std::cerr << "ERROR opening socket\n";
        return connected;
    }

	serv_addr.sin_family = AF_INET;
	//serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

	/* if ((bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) != 0)
	{
		fprintf(stderr, "[Communication_client] ERROR on binding socket at port %i.\n", port);
		exit(1);
	} */

	if (connect(this->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		std::cerr << "ERROR connecting with server\n";
		return connected;
	} 

	char* buffer;
	const char* payload = username.c_str();
	packet pkt;
	pkt.type = 2;
 	pkt.seqn = 0;
  	pkt.total_size = 1;
  	pkt.length = strlen(payload);
	pkt._payload = payload;
    std::cout << "\n\npayload: " << pkt._payload << std::endl;

	buffer = (char*)malloc(this->header_size);
	buffer = (char*)&pkt;

	// Escreve o header no socket
	int bytes_sent = 0;
	while (bytes_sent < this->header_size)
	{
	    n = write(this->sockfd, &buffer[bytes_sent], this->header_size - bytes_sent);
        if (n < 0) {
			std::cerr << "ERROR writing socket\n";
		    return connect;
		}
		bytes_sent += n;
    }
    std::cout << "bytes sent: " << bytes_sent << std::endl;

	// Escreve o payload
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(this->sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) {
			std::cerr << "ERROR writing socket\n";
		    return connect;
		}
		bytes_sent += n;
    }
    std::cout << "bytes sent: " << bytes_sent << std::endl;

	//close(sockfd);
	connected = true;
	return connected;
}


void Communication_client::send_command(int command) {
	int n;
	printf("entrei na send_command\n");

	packet pkt;
	pkt.type = 1;
    pkt.seqn = 0;
    pkt.total_size = 1;
    pkt.length = sizeof(int);
	pkt._payload = (const char*)&command;
	
	// send header
	// write in the socket
	char* buffer = (char*)&pkt;
	int bytes_sent = 0;
	while (bytes_sent < this->header_size)
	{
	    n = write(this->sockfd, &buffer[bytes_sent], this->header_size - bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    std::cout << "bytes sent: " << bytes_sent << std::endl;
    //send payload
	// write in the socket
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(this->sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    std::cout << "bytes sent: " << bytes_sent << std::endl;

	close(this->sockfd);
}
