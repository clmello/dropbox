#include "Servidor.h"
	char *buffer;
	int newsockfd,sockfd;

Servidor::Servidor()
{
	server();
}

void Servidor::server()
{
	int  n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		printf("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(porta[ID]);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		printf("ERROR on binding");

	listen(sockfd, 5);

	clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1)
		printf("ERROR on accept");

	pthread_t th;

	pthread_create(&th,NULL,run,NULL);

}

void *run(void *arg){
	/* write in the socket */

	Pacote msg;

	buffer = (char *)malloc(sizeof(Pacote));

	int bytes_received = 0;
	int pktSize = sizeof(Pacote);

int n;
	/* read from the socket */
	while (bytes_received < pktSize)
	{
		n = read(sockfd, &buffer[bytes_received], pktSize-bytes_received);

		if (n < 0)
			printf("ERROR reading from socket\n");

		bytes_received += n;
	}
/*
	int sourceID;
	int destinID;
	string msg;
	*/
	memcpy(&msg,&buffer,sizeof(msg));
	//msg=(Pacote)buffer;

	Valentao bully;

	if (msg.sourceID != ID)
	{
		if (msg.destinID == liderID)
			bully.lider(msg);

		else
			bully.resto(msg);
	}

//	printf("%s\n", buffer);

	close(sockfd);
		close(newsockfd);

}