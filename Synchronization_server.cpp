#include "Synchronization_server.h"

using namespace std;

// This global variable tells all the threads in the server that the server will close
extern bool closing_server;


Synchronization_server::Synchronization_server(int port)
{
	this->port = port;
	this->header_size = 10;
	this->max_payload = 502;
	this->packet_size = this->header_size + this->max_payload;
	this->buffer = (char*)malloc(packet_size);
	this->buffer_address = (size_t)buffer;
	this->header = (packet*)malloc(header_size);
	this->header_address = (size_t)header;
	//cout << "\nchamando aceita_conexoes\n";
	accept_connections();
}

void *Synchronization_server::accept_connections()
{
    int sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    // Create the socket as non-blocking. Without this, it's impossible for the server to close (since it blocks)
    // Será que precisa? Como que o server é fechado?
    //if ((sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
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
    clilen = sizeof(struct sockaddr_in);
    while(true)
    {
        int newsockfd = -1;
        cout << "\nWaiting for connection . . .\n";
        
        //while(newsockfd<0 && !closing_server)
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(closing_server)
            close_server();

        cout << "CLIENTE DE IP " << cli_addr.sin_port << " CONECTADO!";
        
        // Receive username from client
        struct packet *pkt = receive_payload(newsockfd);
        string str_buff = pkt->_payload;
        string new_client_username = str_buff.substr(0, pkt->length);
		
		// Check if the client is already connected. If it is, check if number of connections > max_connections
		// num_connections will be -1 if number of connections > max_connections
		int num_connections=1;
        for(int i=0; i<connected_clients.size(); i++){
            cout << "\nverificando username \"" << new_client_username << "\" contra \"" << *connected_clients[i].get_username() << "\"";
            if(new_client_username == *connected_clients[i].get_username())
                num_connections = connected_clients[i].new_connection();
        }
            
	    // Create the new connected client
	    Connected_client new_client(new_client_username, newsockfd, num_connections, port, header_size, max_payload);
	    cout << "\n&buffer de " << new_client_username << ": ";
        //printf("%p\n", &new_client.com.buffer);
        
        if(num_connections < 0){ // Too many connections
            cout << "\nClient " << new_client_username << " failed to connect. Too many connections." << endl;
            // Tell the client to close
            new_client.com.send_int(newsockfd, -1);
        }
		else
		{
		    cout << "\nClient " << new_client_username << " connected" << endl;
		    
		    // Tell the client that the connection has been accepted
		    new_client.com.send_int(newsockfd, 1);
		
		    // Create client folder, if it doesn't already exist
		    string homedir = getenv("HOME");
		    new_client.com.create_folder(homedir+"/server_sync_dir_"+new_client_username);

            // Create a struct with the arguments to be sent to the new thread
            struct th_args args;
            args.obj = &new_client.com;
            args.newsockfd = new_client.get_sockfd();
            args.username = new_client.get_username();
            // TODO: Mandar vector pra thread pra que ela possa se retirar quando terminar. Não esquecer de decrementar as conexões ativas
            //args.connected_clients = &connected_clients;

// TODO: mutex pra que dois clientes do mesmo user não possam fazer upload e download do mesmo arquivo ao mesmo tempo
// TODO: arrumar memory leaks do communication_server
            // Create the thread for this client
		    pthread_t client_thread;
            pthread_create(&client_thread, NULL, new_client.com.receive_commands_helper, &args);
            
            // Set the new connected client thread and add it to the connected_clients vector
            new_client.set_thread(client_thread);
		    connected_clients.push_back(new_client);
        }
    }
}

void Synchronization_server::close_server()
{
    cout << "\n\nENTREI NO CLOSE_SERVER";
    cout << "\ntamanho do connected_clients: " << connected_clients.size();
    // Close all client sockets and join all client threads
    for(int i=0; i<connected_clients.size(); i++){
        cout << "\n\ni: " << i;
        cout << "\njoining thread " << connected_clients[i].get_thread();
        pthread_join(connected_clients[i].get_thread(), NULL);
        cout << "\nclosing socket " << *connected_clients[i].get_sockfd();
        close(*connected_clients[i].get_sockfd());
        cout << "\nDONE!";
    }
    cout << "\n\nEND!\n\n";
    exit(0);
}

packet* Synchronization_server::receive_header(int sockfd)
{
    buffer = (char*)buffer_address;
    header = (packet*)header_address;
    
	int bytes_received=0;
    while(bytes_received < header_size)
    {
        int n = read(sockfd, buffer, header_size-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
	}
	if(bytes_received != 0) // No need to copy anything to the header if no bytes were received
	{
	    memcpy(&header->type, &buffer[0], 2);
	    memcpy(&header->seqn, &buffer[2], 2);
	    memcpy(&header->total_size, &buffer[4], 4);
	    memcpy(&header->length, &buffer[8], 2);
    }
	
	return header;
}

packet* Synchronization_server::receive_payload(int sockfd)
{
    struct packet *pkt = receive_header(sockfd);
	int bytes_received=0;
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
	}
	pkt->_payload = (const char*)buffer;
	return pkt;
}















