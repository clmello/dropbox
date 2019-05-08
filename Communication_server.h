#ifndef COMMUNICATION_SERVER_H
#define COMMUNICATION_SERVER_H
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


class Communication_server
{
	public:
		typedef	struct	packet{
			uint16_t	type;		//Tipo do pacote (p.ex. DATA | CMD)
			uint16_t	seqn;		//Número de sequência
			uint32_t	total_size;		//Número total de fragmentos
			uint16_t	length;	//Comprimento do payload
			const	char*	_payload;				//Dados do pacote
		}	packet;	
		Communication_server(int port);
		void test();

	protected:

	private:
		int port;
		char buffer[8000];
		pthread_t th_conexoes, th_comandos;
		
		void *accept_connections();
		void *receive_commands(int newsockfd);



		struct th_args{
			void* obj = NULL;
			int* newsockfd = NULL;
		};
		static void *receive_commands_helper(void *void_args);
};

#endif // COMMUNICATION_SERVER_H
