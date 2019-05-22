#ifndef COMMUNICATION_CLIENT_H
#define COMMUNICATION_CLIENT_H
#include "stdint.h"
#include "client.h"

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

typedef	struct	packet{
		uint16_t	type;			//Tipo do pacote (p.ex. DATA | CMD)
		uint16_t	seqn;			//Número de sequência
		uint32_t	total_size;		//Número total de fragmentos
		uint16_t	length;			//Comprimento do payload
		const char*	_payload;		//Dados do pacote
	} packet;

class Communication_client {
private:
	int payload_size;
	int header_size;
	int packet_size;
	int sockfd;
	char* buffer;
	packet* header;
	size_t header_address;
	size_t buffer_address;

public:

	Communication_client();
	bool connect_client_server(Client client);
	void send_command(int command);
	void send_filename(std::string filename);
	void send_file(std::string filename, std::string path);
	void send_mtime(time_t mtime);
 	packet* receive_header();
	packet* receive_payload();
	void receive_file(std::string path);
	int receive_int();
	long get_file_size(FILE *fp);
	int delete_file(std::string path);

	void upload_command(int command, std::string filename, std::string path, time_t mtime);
	Client::file download_command(int command, std::string filename, std::string path, Client::file download_file);
	void delete_command(int command, std::string filename, std::string path);
	void list_server_command(int command);
	void exit_command(int command);
};

#endif // COMMUNICATION_CLIENT_H
