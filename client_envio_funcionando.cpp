#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <iostream>

#define PORT 4002

int payload_size = 502;
int header_size = 10;
int packet_size = header_size + payload_size;

typedef	struct	packet{
	uint16_t	type;		//Tipo do pacote (p.ex. DATA | CMD)
	uint16_t	seqn;		//Número de sequência
	uint32_t	total_size;		//Número total de fragmentos
	uint16_t	length;	//Comprimento do payload
	char	_payload[502];				//Dados do pacote
}	packet;	

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
    char buffer[packet_size];
    if (argc < 2) {
		fprintf(stderr,"usage %s hostname\n", argv[0]);
		exit(0);
    }
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket\n");
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
	
    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        printf("ERROR connecting\n");

    bzero(buffer, packet_size);
    	
    struct packet pkt;
    pkt.type = 42;
    pkt.seqn = 55;
    pkt.total_size = 67;
    pkt.length = 35;
	memcpy(pkt._payload, "isto é um teste", sizeof("isto é um teste"));
    std::cout << "\n\ntamanho: " << sizeof(pkt) << std::endl;
	/* write in the socket */
	int bytes_sent = 0;
	memcpy(&buffer, &pkt, packet_size);
	while (bytes_sent < packet_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], packet_size-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    
	close(sockfd);
    return 0;
}
