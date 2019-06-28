#ifndef CLIENT
#define CLIENT
#include "Pacote.h"
#include "Valentao.h"
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>

class Cliente
{
public:
	void run();
	static void sendToOthers(Pacote msg);
	static void sendToLider(Pacote msg);

};

#endif