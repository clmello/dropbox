
#include "Cliente.h"

void Cliente::run()
{
	Valentao bully;
	bully.eleicao(ID);
}

void Cliente::sendToOthers(Pacote msg)
{
	/* read from the socket */

	//cout<<"foi env\n";
	char *buffer;

	 int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
	
	server = gethostbyname("localhost");
	cout<<"passou";
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket\n");
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(porta[msg.destinID]);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
	
    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        printf("ERROR connecting Others\n");

    printf("Enter the message: ");

	/* write in the socket */
	buffer = (char *)&msg;
	int bytes_sent = 0;

	int pktSize = sizeof(Pacote);

	while (bytes_sent < pktSize)
	{
		n = write(sockfd, &buffer[bytes_sent],pktSize-bytes_sent);
		if (n < 0)
		{
			cout << "Processo"<<msg.destinID<<" caiu!";
		
		}

		bytes_sent += n;
	}
		close(sockfd);
}

void Cliente::sendToLider(Pacote msg)
{
	/* read from the socket */

	char *buffer;

	 int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
	
	server = gethostbyname("localhost");
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket\n");
    
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(porta[msg.destinID]);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);     
	
    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        printf("ERROR connecting\n");

    printf("Enter the message: ");

	/* write in the socket */
	buffer = (char *)&msg;
	int bytes_sent = 0;

	int pktSize = sizeof(Pacote);

	while (bytes_sent < pktSize)
	{
		n = write(sockfd, &buffer[bytes_sent],pktSize-bytes_sent);
		if (n < 0)
		{
			cout << "Lider caiu!";
			Valentao bully;
			bully.eleicao(msg.sourceID);
		}

		bytes_sent += n;
	}
		close(sockfd);
}

