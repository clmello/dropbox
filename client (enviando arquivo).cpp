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

#define PORT 4001

int payload_size = 502;
int header_size = 10;
int packet_size = header_size + payload_size;

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





FILE* read_file(string path)
{
    char ch, file_name[25];
    FILE *fp;
    
    fp = fopen(path.c_str(), "r"); // read mode
    
    if(fp == NULL)
        cout << "Error opening file " << path << endl;
    
    return fp;
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
	
    char* buffer;
    char* file_buffer = (char*)malloc(payload_size);
    const char* payload = (char*)"bla";
    
    // Create the packet that will be sent
    struct packet pkt;
    pkt.type = 2;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = 5;
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
	buffer = (char*)malloc(header_size);
	buffer = (char*)&pkt;
	
	cout << "\n\nmemória enviada: \n";
	print_bytes(buffer, header_size);
	cout << "\n\nsending: \n";
	struct packet* header;
	header = (packet*)malloc(header_size);
	memcpy(&header->type, &buffer[0], 2);
	memcpy(&header->seqn, &buffer[2], 2);
	memcpy(&header->total_size, &buffer[4], 4);
	memcpy(&header->length, &buffer[8], 2);
	cout << "type: " << header->type << endl;
	cout << "seqn: " << header->seqn << endl;
	cout << "total_size: " << header->total_size << endl;
	cout << "payload_size: " << header->length << endl << endl;
	
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
    
    
    
    
    //------------------------------------------------------------------------
    // SEND COMMAND
    //------------------------------------------------------------------------
    // This send was created to test sending an integer.
    
    sleep(5);
    pkt.type = 1;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(int);
    int command = 1;
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
    
    
    
    
    // To send a file, you must first send the file name, and then the file
    // Of course, you also need to send the upload command before the file name
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
    
    //------------------------------------------------------------------------
    // SEND FILE
    //------------------------------------------------------------------------
    string path = "/home/alack/Downloads/sync_dir_vinicius/teste.txt";
    FILE *fp = fopen(path.c_str(), "r");
    if(fp == NULL)
        cout << "Error opening file " << path << endl;
    
    // Get the size of the file
    fseek(fp, 0 , SEEK_END);
    long total_payload_size = ftell(fp);
    // Go back to the beggining
    fseek(fp, 0 , SEEK_SET);
    
    // The type of the packet being sent is 0 (data)
    uint16_t type = 0;
    
    // If the data is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)total_payload_size/(float)payload_size;
    int total_size = total_size_f;
    if (total_size_f > total_size)
        total_size ++;
    cout << "\n\ntotal size: " << total_size;
    
    int i;
    int total_bytes_sent = 0;
    cout << "\n\nenviando: " << endl;
	printf("%.*s\n", payload_size, buffer);
    
    // Send each packet
    // If only one packet will be sent, the program will go through the loop only once
    for(i=1; i<=total_size; i++)
    {
        // Create the packet that will be sent
        struct packet pkt;
        pkt.type = type;
        pkt.seqn = i;
        pkt.total_size = total_size;
        
        // If the chunk of the file that will be sent is smaller
        //than the max payload size, send only the size needed
        if(payload_size > total_payload_size - (total_bytes_sent - header_size*(i-1)))
            pkt.length = total_payload_size - (total_bytes_sent - header_size*(i-1));
        else
            pkt.length = payload_size;
        
        cout << endl << total_bytes_sent << " bytes have been sent";
        cout << endl << total_payload_size - (total_bytes_sent - header_size*(i-1)) << " bytes will be sent";
        
        // Read pkt.length bytes from the file
        fread(file_buffer, 1, pkt.length, fp);
        // Save it to pkt._payload
        pkt._payload = file_buffer;
        
        // Point buffer to pkt
        buffer = (char*)&pkt;
        
        //------------------------------------------------------------------------
        // SEND HEADER
        //------------------------------------------------------------------------
        // write in the socket
        int bytes_sent = 0;
        while (bytes_sent < header_size)
        {
            int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
            if (n < 0) 
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        cout << "\n\nHEADER!\n";
        cout << "bytes sent: " << bytes_sent << endl;
        cout << "type: " << pkt.type;
        cout << "\nseqn: " << pkt.seqn;
        cout << "\ntotal_size: " << pkt.total_size;
        cout << "\npayload_size: " << pkt.length << endl;
        
        //------------------------------------------------------------------------
        // SEND PAYLOAD
        //------------------------------------------------------------------------
        // write in the socket
        bytes_sent = 0;
        while (bytes_sent < pkt.length)
        {
            int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
            if (n < 0) 
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        cout << "PACKET!\n";
        cout << "\npayload(char*): ";
        printf("%.*s\n", payload_size, pkt._payload);
        cout << "bytes sent: " << bytes_sent << endl;
        //------------------------------------------------------------------------
    }
    
    fclose(fp);
	close(sockfd);
	
    return 0;
}

