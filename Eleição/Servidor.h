#ifndef SERVER
#define SERVER
#include "Pacote.h"
#include "Valentao.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

	void *run(void *arg);


class Servidor
{
public:

	Servidor();
	void server();

};
#endif
