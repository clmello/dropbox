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


bool Communication_client::connect_client_server(Client client) {
	int sockfd, n;
	int port = client.getPort();
	std::string hostname = client.getHostname();
	bool connected = false;
    struct sockaddr_in serv_addr; // server_address
    struct hostent *server = gethostbyname(hostname.c_str());

	char buffer[256];
	
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
		std::cerr << "ERROR connecting\n";
		return connected;
	}

    printf("Enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 256, stdin);
    
	/* write in the socket */
	struct packet pacote_envio;
	pacote_envio.type = 1;
	pacote_envio.seqn=2;
	pacote_envio.length=5;
	pacote_envio._payload = "teste";
	pacote_envio.total_size = sizeof(pacote_envio) + sizeof(sizeof(pacote_envio));
	int bytes_sent = 0;
	while (bytes_sent < sizeof(pacote_envio))
	{
	    n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) 
		    printf("ERROR writing to socket\n");
    }

    bzero(buffer,256);
	
	/* read from the socket */
    n = read(sockfd, buffer, 256);
    if (n < 0) 
		printf("ERROR reading from socket\n");

    printf("%s\n",buffer);
    
	close(sockfd);
    return true;
}
