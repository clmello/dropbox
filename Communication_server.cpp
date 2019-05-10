#include "Communication_server.h"

using namespace std;

Communication_server::Communication_server(int port)
{
	this->port = port;
	this->header_size = 10;
	this->max_payload = 502;
	cout << "\nchamando aceita_conexoes\n";
	accept_connections();
}

void *Communication_server::accept_connections()
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    //while(true) // TODO: ENQUANTO USUARIO NÃO FECHA
    //{
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            printf("ERROR opening socket");
            cout << "\nsocket aberto\n";

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(serv_addr.sin_zero), 8);

        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            printf("ERROR on binding");
            cout << "\nbinding completo\n";


        listen(sockfd, 5);

        cout << "\nesperando conexao\n";
        clilen = sizeof(struct sockaddr_in);
        if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
            printf("ERROR on accept");

        cout << "CLIENTE DE IP " << cli_addr.sin_port << " CONECTADO!";

        // Create a struct with the arguments to be sent to the new thread
        struct th_args args;
        args.obj = this;
        args.newsockfd = &newsockfd;
		
		// Create the new connected client and add it to the connected_clients vector
		pthread_t client_thread;
		Connected_client new_client(client_thread, receive_payload(newsockfd)->_payload, newsockfd);
		connected_clients.push_back(new_client);
		cout << "\nUsername: " << connected_clients[0].get_username() << endl;
		
		// Create client folder, if it doesn't already exist
		create_folder("/home/"+connected_clients[connected_clients.size()-1].get_username()+"_syncdir");

        //pthread_create(&client_thread, NULL, receive_commands_helper, &args);
        //pthread_join(client_thread,NULL);
    //}

}

packet* Communication_server::receive_header(int sockfd)
{
	int bytes_received=0;
    bzero(buffer, header_size);
	cout << "\nbytes lidos: "<<bytes_received<<endl;
    while(bytes_received < header_size)
    {
        cout << "\n\nsockfd = " << sockfd << "\n\n";
        /* read from the socket */
        int n = read(sockfd, buffer, header_size);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	// Bytes from buffer[4] to buffer[7] are the size of _payload
	struct packet* header;
	header = (packet*) malloc( sizeof(packet) );
	memcpy(&header->type, &buffer, 2);
	memcpy(&header->seqn, &buffer[2], 2);
	memcpy(&header->total_size, &buffer[4], 4);
	memcpy(&header->length, &buffer[8], 2);
	cout << "\n\npayload_size: " << header->length << endl << endl;
	
	return header;
}

packet* Communication_server::receive_payload(int sockfd)
{
    struct packet *pkt = receive_header(sockfd);
	int bytes_received=0;
    bzero(buffer, pkt->length);
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	memcpy(&pkt->_payload, &buffer, sizeof(buffer));
	cout << "\npayload: " << pkt->_payload << endl;
	
	return pkt;
}

void *Communication_server::receive_commands(int newsockfd)
{
    while(true) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        
	}
}

void *Communication_server::receive_commands_helper(void* void_args)
{
    th_args* args = (th_args*)void_args;
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd);
    return 0;
}

int Communication_server::create_folder(string path)
{
    DIR* dir = opendir(path.c_str());
    cout << "\npath: " << path << endl;
    if(!dir)
    {
        string command = "mkdir -p " + path;
        int error = system(command.c_str());
        if(error < 0)
            return -1;
    }
    return 0;
}

int Communication_server::delete_folder(string path)
{
    DIR* dir = opendir(path.c_str());
    if(dir)
    {
        string command = "rm -r " + path;
        cout << "\ncommand: " << command << endl;
        int error = system(command.c_str());
        if(error < 0)
            return -1;
    }
    return 0;
}














