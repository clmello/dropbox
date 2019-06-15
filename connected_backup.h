#ifndef CONNECTED_BACKUP_H
#define CONNECTED_BACKUP_H

#include "Communication_server.h"

#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>

using namespace std;

class Connected_backup{

	public:
		Connected_backup(int sockfd, int port, int header_size, int max_payload, int* server_closed);
        void* heartbeat();

		Communication_server com;

	private:
		int sockfd;
		int* server_closed;
};

#endif // CONNECTED_BACKUP_H
