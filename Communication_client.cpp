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
	this->packet_size = payload_size + header_size;
}

bool Communication_client::connect_client_server(Client client) {
	int sockfd, n;
	int port = client.getPort();
	std::string hostname = client.getHostname();
	std::string username = client.getUsername();
	bool connected = false;
    struct sockaddr_in serv_addr; // server_address
    struct hostent *server = gethostbyname(hostname.c_str());

	char buffer[this->packet_size];
	
	if (server == NULL) {
		std::cerr << "ERROR, no such host\n";
        return connected;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
		std::cerr << "ERROR opening socket\n";
        return connected;
    }

	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(port); 
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);
	
    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		std::cerr << "ERROR connecting with server\n";
		return connected;
	}

	bzero(buffer, this->packet_size);
	struct packet pkt;
	pkt.type = 42;
    pkt.seqn = 55;
    pkt.total_size = 67;
    pkt.length = 35;

	memcpy(pkt._payload, username.c_str(), sizeof(username));
    std::cout << "\n\ntamanho: " << sizeof(pkt) << std::endl;

	// Escreve no socket
	int bytes_sent = 0;
	memcpy(&buffer, &pkt, this->packet_size);

	while (bytes_sent < pkt.length + this->header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], pkt.length + this->header_size - bytes_sent);
        if (n < 0) {
			std::cerr << "ERROR writing to socket\n";
			return connected;
		}
		bytes_sent += n;
    }
    
	close(sockfd);
	connected = true;
    return connected;
}
