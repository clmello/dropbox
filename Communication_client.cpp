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
	this->buffer = (char*)malloc(header_size);
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
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

	if (connect(this->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		std::cerr << "ERROR connecting with server\n";
		return connected;
	} 

	// SEND USERNAME
	const char* payload = username.c_str();
	packet pkt;
	pkt.type = 2;
 	pkt.seqn = 0;
  	pkt.total_size = 1;
  	pkt.length = strlen(payload);
	pkt._payload = payload;
    std::cout << "\n\npayload: " << pkt._payload << std::endl;

	this->buffer = (char*)&pkt;

	// Escreve o header no socket
	int bytes_sent = 0;
	while (bytes_sent < this->header_size)
	{
	    n = write(this->sockfd, &this->buffer[bytes_sent], this->header_size - bytes_sent);
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
	this->buffer = (char*)&pkt;
	int bytes_sent = 0;
	while (bytes_sent < this->header_size)
	{
	    n = write(this->sockfd, &this->buffer[bytes_sent], this->header_size - bytes_sent);
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
}

void Communication_client::upload_command(int command, std::string filename, std::string path) {

	//send command upload (1)
	send_command(command);

	// send filename
	send_filename(filename);

	//send file
	send_file(filename, path);

}


void Communication_client::send_filename(std::string filename) {
	int n;
	int bytes_sent = 0;
	packet pkt;
	const char* payload = filename.c_str();
	// Create the packet that will be sent
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = 9;
	pkt._payload = payload;
    std::cout << "\n\nfilename: " << pkt._payload << std::endl;
    
	// copy pkt to buffer
	this->buffer = (char*)&pkt;
	
	// send header
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < this->header_size)
	{
	    n = write(this->sockfd, &this->buffer[bytes_sent], this->header_size - bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    std::cout << "bytes sent: " << bytes_sent << std::endl;
    
    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(this->sockfd, &pkt._payload[bytes_sent], pkt.length - bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    std::cout << "bytes sent: " << bytes_sent << std::endl;
}

void Communication_client::send_file(std::string filename, std::string path){
	char* file_buffer = (char*)malloc(this->payload_size);
	std::string complete_path = path + "/" + filename;
	std::cout << "PATH COMPLETO AAAH:" << complete_path;
	FILE *fp = fopen(complete_path.c_str(), "r");
    
	if(fp == NULL)
        std::cout << "Error opening file " << path << std::endl;
    
    // Get the size of the file
    fseek(fp, 0 , SEEK_END);
    long total_payload_size = ftell(fp);
    // Go back to the beggining
    fseek(fp, 0 , SEEK_SET);
    
    // The type of the packet being sent is 0 (data)
    uint16_t type = 0;
    
    // If the data is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)total_payload_size/(float)this->payload_size;
    int total_size = total_size_f;
    if (total_size_f > total_size)
        total_size ++;
    std::cout << "\n\ntotal size: " << total_size;
    
    int i;
    int total_bytes_sent = 0;
    std::cout << "\n\nenviando: " << std::endl;
	printf("%.*s\n", this->payload_size, this->buffer);
    
    // Send each packet
    // If only one packet will be sent, the program will go through the loop only once
    for(i=1; i<=total_size; i++)
    {
        // Create the packet that will be sent
        packet pkt;
        pkt.type = type;
        pkt.seqn = i;
        pkt.total_size = total_size;
        
        // If the chunk of the file that will be sent is smaller
        //than the max payload size, send only the size needed
        if(this->payload_size > total_payload_size - (total_bytes_sent - this->header_size*(i-1)))
            pkt.length = total_payload_size - (total_bytes_sent - this->header_size*(i-1));
        else
            pkt.length = this->payload_size;
        
        std::cout << std::endl << total_bytes_sent << " bytes have been sent";
        std::cout << std::endl << total_payload_size - (total_bytes_sent - this->header_size*(i-1)) << " bytes will be sent";
        
        // Read pkt.length bytes from the file
        fread(file_buffer, 1, pkt.length, fp);
        // Save it to pkt._payload
        pkt._payload = file_buffer;
        
        // Point buffer to pkt
       this-> buffer = (char*)&pkt;
        
        //------------------------------------------------------------------------
        // SEND HEADER
        //------------------------------------------------------------------------
        // write in the socket
        int bytes_sent = 0;
        while (bytes_sent < this->header_size)
        {
            int n = write(this->sockfd, &this->buffer[bytes_sent], this->header_size - bytes_sent);
            if (n < 0) 
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        std::cout << "\n\nHEADER!\n";
        std::cout << "bytes sent: " << bytes_sent << std::endl;
        std::cout << "type: " << pkt.type;
        std::cout << "\nseqn: " << pkt.seqn;
        std::cout << "\ntotal_size: " << pkt.total_size;
        std::cout << "\npayload_size: " << pkt.length << std::endl;
        
        //------------------------------------------------------------------------
        // SEND PAYLOAD
        //------------------------------------------------------------------------
        // write in the socket
        bytes_sent = 0;
        while (bytes_sent < pkt.length)
        {
            int n = write(this->sockfd, &pkt._payload[bytes_sent], pkt.length - bytes_sent);
            if (n < 0) 
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        std::cout << "PACKET!\n";
        std::cout << "\npayload(char*): ";
        printf("%.*s\n", this->payload_size, pkt._payload);
        std::cout << "bytes sent: " << bytes_sent << std::endl;
    }
	
	fclose(fp);
}


void Communication_client::list_server_command(int command) {
    send_command(command);

    std::cout << "\n\nlist_server: " << receive_payload(this->sockfd)->_payload << std::endl << std::endl;
}

Communication_client::packet* Communication_client::receive_payload(int sockfd) {
    packet *pkt = receive_header(this->sockfd);
	int bytes_received=0;
    bzero(buffer, pkt->length);
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(this->sockfd, this->buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		//cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	std::cout << "\nbytes lidos: " << bytes_received;
	pkt->_payload = (const char*)this->buffer;
	if(pkt->type != 1){ // If the packet is not a command
	    std::cout << "\npayload(char*): ";
	    printf("%.*s\n", this->payload_size, pkt->_payload);
    }
    else{ // If the packet is a command
	    std::cout << "payload(int): ";
        int command;
        memcpy(&command, pkt->_payload, pkt->length);
        std::cout << command;
    }
    std::cout << std::endl << std::endl;
	return pkt;
}

Communication_client::packet* Communication_client::receive_header(int sockfd) {
	int bytes_received=0;
    bzero(this->buffer, this->header_size);
	std::cout << "\n\nbytes lidos: " << bytes_received;
    while(bytes_received < this->header_size)
    {
        //cout << "\n\nsockfd = " << sockfd << "\n\n";
        // read from the socket
        int n = read(this->sockfd, this->buffer, this->header_size);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		std::cout << "\nbytes lidos: "<< bytes_received;
	}
	// Bytes from buffer[4] to buffer[7] are the size of _payload
	packet* header;
	header = (packet*)malloc(this->header_size);
	memcpy(&header->type, &this->buffer[0], 2);
	memcpy(&header->seqn, &this->buffer[2], 2);
	memcpy(&header->total_size, &this->buffer[4], 4);
	memcpy(&header->length, &this->buffer[8], 2);
	std::cout << "\ntype: " << header->type;
	std::cout << "\nseqn: " << header->seqn;
	std::cout << "\ntotal_size: " << header->total_size;
	std::cout << "\npayload_size: " << header->length << std::endl;
	
	return header;
}


void Communication_client::exit_command(int command) {
    send_command(7);

    close(this->sockfd);
}
