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


class Communication_client {
private:
	int payload_size;
	int header_size;
	int packet_size;

public:
	typedef	struct	packet{
		uint16_t	type;			//Tipo do pacote (p.ex. DATA | CMD)
		uint16_t	seqn;			//Número de sequência
		uint32_t	total_size;		//Número total de fragmentos
		uint16_t	length;			//Comprimento do payload
		char		_payload[502];	//Dados do pacote
	} packet;

	Communication_client();
	bool connect_client_server(Client client);
};

#endif // COMMUNICATION_CLIENT_H
