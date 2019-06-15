#include "connected_backup.h"

Connected_backup::Connected_backup(int sockfd, int port, int header_size, int max_payload, int* server_closed)
{
    this->sockfd = sockfd;
    this->server_closed = server_closed;
    com.Init(port, header_size, max_payload);
}

void Connected_backup::heartbeat()
{
    while(!*server_closed == 1)
    {
        com.send_string(sockfd, "alive");

        struct packet *pkt;
        com.receive_payload(sockfd, pkt, 0);
        string str_buff = pkt->_payload;
        string response = str_buff.substr(0, pkt->length);

        if(response != "Ok")
            cout << "\nHouston . . .";

        sleep(10);
    }
}
