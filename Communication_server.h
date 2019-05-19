#ifndef COMMUNICATION_SERVER_H
#define COMMUNICATION_SERVER_H
#include "connected_client.h"
#include "stdint.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

typedef	struct	packet{
	uint16_t	type;		//Tipo do pacote (p.ex. DATA | CMD)
	uint16_t	seqn;		//Número de sequência
	uint32_t	total_size;		//Número total de fragmentos
	uint16_t	length;	//Comprimento do payload
	const char*	_payload;				//Dados do pacote
}	packet;	


class Communication_server
{
	public:
		Communication_server(int port);

	protected:

	private:
		int port;
		int header_size;
		int max_payload;
		int packet_size;
		char* buffer;
		size_t buffer_address;
		struct packet* header;
		size_t header_address;
//		char output[512];
		vector<pthread_t> client_threads;
		vector<Connected_client> connected_clients;
		string username;
		
		void *accept_connections();
		void *receive_commands(int sockfd);
		
		packet* receive_payload(int sockfd);// Receives the _payload of the packet from the client and returns a packet struct containing the _payload
		packet* receive_header(int sockfd);	// Receives the header of the packet from the client and returns a packet struct containing the header
		void send_file(int sockfd, string file_name, string path); // Send a file to the client
		void send_string(int sockfd, string str); // Send a string to the client
		void receive_file(int sockfd, string path); // Receive a file from the client
		
		int create_folder(string path);
		int delete_folder(string path);
		int delete_file(string path);
		long get_file_size(FILE *fp);


		struct th_args{
			void* obj = NULL;
			int* newsockfd = NULL;
		};
		static void *receive_commands_helper(void *void_args);
};

#endif // COMMUNICATION_SERVER_H
