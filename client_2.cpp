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
#include <ctime>

#define PORT 4001

// Essas coisas globais precisam ser variáveis privadas da classe
int payload_size = 502;
int header_size = 10;
int packet_size = header_size + payload_size;
char* buffer = (char*)malloc(header_size);

using namespace std;

typedef	struct	packet{
	uint16_t	type;		//Tipo do pacote (p.ex. DATA | CMD)
	uint16_t	seqn;		//Número de sequência
	uint32_t	total_size;		//Número total de fragmentos
	uint16_t	length;	//Comprimento do payload
	const char*	_payload;				//Dados do pacote
}	packet;	

void print_bytes(const void *object, size_t size)
{
  // This is for C++; in C just drop the static_cast<>() and assign.
  const unsigned char * const bytes = static_cast<const unsigned char *>(object);
  size_t i;

  printf("[ ");
  for(i = 0; i < size; i++)
  {
    printf("%02x ", bytes[i]);
  }
  printf("]\n");
}

packet* receive_header(int sockfd)
{
	int bytes_received=0;
    bzero(buffer, header_size);
	cout << "\n\nbytes lidos: "<<bytes_received;
    while(bytes_received < header_size)
    {
        //cout << "\n\nsockfd = " << sockfd << "\n\n";
        /* read from the socket */
        int n = read(sockfd, buffer, header_size);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		cout << "\nbytes lidos: "<<bytes_received;
	}
	// Bytes from buffer[4] to buffer[7] are the size of _payload
	struct packet* header;
	header = (packet*)malloc(header_size);
	memcpy(&header->type, &buffer[0], 2);
	memcpy(&header->seqn, &buffer[2], 2);
	memcpy(&header->total_size, &buffer[4], 4);
	memcpy(&header->length, &buffer[8], 2);
	cout << "\ntype: " << header->type;
	cout << "\nseqn: " << header->seqn;
	cout << "\ntotal_size: " << header->total_size;
	cout << "\npayload_size: " << header->length << endl;
	
	return header;
}

packet* receive_payload(int sockfd)
{
    struct packet *pkt = receive_header(sockfd);
	int bytes_received=0;
    bzero(buffer, pkt->length);
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		//cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	cout << "\nbytes lidos: "<<bytes_received;
	pkt->_payload = (const char*)buffer;
	if(pkt->type != 1){ // If the packet is not a command
	    cout << "\npayload(char*): ";
	    printf("%.*s\n", payload_size, pkt->_payload);
    }
    else{ // If the packet is a command
	    cout << "payload(int): ";
        int command;
        memcpy(&command, pkt->_payload, pkt->length);
        cout << command;
    }
    cout << endl << endl;
	return pkt;
}

int receive_int(int sockfd)
{
    struct packet *pkt = receive_payload(sockfd);
    int integer;
    memcpy(&integer, pkt->_payload, pkt->length);
    return integer;
}


void receive_file(int sockfd, string path)
{
    FILE *fp = fopen(path.c_str(), "w");
    if(fp==NULL)
        cout << "\nERROR OPENING " << path << endl; 

    // Get the number of packets to be received
    // To do that, we must receive the first packet
    struct packet *pkt = receive_payload(sockfd);
    uint32_t total_size = pkt->total_size;
    cout << "\n\nTHE CLIENT WILL RECEIVE " << total_size << " PACKETS!\n";
    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
    if (bytes_written_to_file < pkt->length)
        cout << "\nERROR WRITING TO " << path << endl;
    
    //cout << bytes_written_to_file << " bytes written to file" << endl;
    
    // Receive all the [total_size] packets
    // It starts at 2 because the first packet has already been received
    int i;
    for(i=2; i<=total_size; i++)
    {
        // Receive payload
        receive_payload(sockfd);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
        if (bytes_written_to_file < pkt->length)
            cout << "\nERROR WRITING TO " << path << endl;
        //cout << "\n" << bytes_written_to_file << " bytes written to file\n";
    }
    cout << "\nEU QUERO";
    fclose(fp);
    cout << " DORMIR\n";
}












int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
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
	
    char* file_buffer = (char*)malloc(payload_size);
    const char* payload = (char*)"bla";
    
    // Create the packet that will be sent
    struct packet pkt;
    pkt.type = 2;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = 3;
	pkt._payload = payload;
    std::cout << "\n\npayload: " << pkt._payload << std::endl;
    








    //------------------------------------------------------------------------
	// SEND USERNAME
    //------------------------------------------------------------------------
    // Two sends are necessary because pkt._payload is just a pointer. If we
    //sent it, the receiver would just have a memory address that it cannot
    //access. Therefore, first we send the header, and then we send the data
    //that pkt._payload points to.
    
	// copy pkt to buffer
	buffer = (char*)&pkt;
	
	// send header
	/* write in the socket */
	int bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    
    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //------------------------------------------------------------------------
    
    // Receive return int
    if(receive_int(sockfd) < 0){
        cout << "\nConnection refused\nToo many connections\n";
        exit(0);
    }
    
    
    
    
    
    
    
    
    // TESTING DOWNLOAD
    cout << "\nsending command 2\n";
    
    //------------------------------------------------------------------------
    // SEND COMMAND
    //------------------------------------------------------------------------
    // This send was created to test sending an integer.
    
    //sleep(5);
    pkt.type = 1;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(int);
    int command = 2;
	pkt._payload = (const char*)&command;
	// send header
	/* write in the socket */
	buffer = (char*)&pkt;
	bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //------------------------------------------------------------------------
    
    // Receive return int
    if(receive_int(sockfd) < 0){
        cout << "\nServer closed\n";
        exit(0);
    }
    
    cout << "\nsending filename\n";
    //------------------------------------------------------------------------
    // SEND FILENAME
    //------------------------------------------------------------------------
    payload = (char*)"teste.txt";
    
    // Create the packet that will be sent
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = 9;
	pkt._payload = payload;
    std::cout << "\n\nfilename: " << pkt._payload << std::endl;
    
	// copy pkt to buffer
	buffer = (char*)&pkt;
	
	// send header
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    
    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //------------------------------------------------------------------------
    
    cout << "\nreceiving mtime\n";
    //------------------------------------------------------------------------
    // RECEIVE MTIME
    //------------------------------------------------------------------------
    // This was created to test receiving mtime
    receive_payload(sockfd);
    time_t mtime = *(time_t*)pkt._payload;
    //------------------------------------------------------------------------
    
    cout << "\nreceiving file\n";
    //------------------------------------------------------------------------
    // RECEIVE FILE
    //------------------------------------------------------------------------
    // This was created to test receiving a file
    receive_file(sockfd, "/home/alack/sync_dir_bla/teste.txt");
    //------------------------------------------------------------------------
    
    cout << "\nFILE RECEIVED\n";
    
    
    
    
    
    
    
    /*cout << "\nsleeping";
    sleep(5);
    cout << "\nwoke up\n";*/
    
    
    // TESTING EXIT
    
    //------------------------------------------------------------------------
    // SEND COMMAND
    //------------------------------------------------------------------------
    // This send was created to test sending an integer.
    
    //sleep(5);
    pkt.type = 1;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(int);
    command = 7;
	pkt._payload = (const char*)&command;
	// send header
	/* write in the socket */
	buffer = (char*)&pkt;
	bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //------------------------------------------------------------------------
    
    cout << "\nwaiting return";
    
    // Receive return int
    if(receive_int(sockfd) < 0){
        cout << "\nServer closed\n";
        exit(0);
    }
    
    
    cout << "\n\nEND!\n\n";
	close(sockfd);
	
    return 0;
}

