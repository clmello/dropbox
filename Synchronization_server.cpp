#include "Synchronization_server.h"

using namespace std;

// This global variable tells all the threads in the server that the server will close
extern bool closing_server;


Synchronization_server::Synchronization_server(int port, int backup_port, string main_ip, int main_port)
{
	this->port = port;
	this->backup_port = backup_port;
	this->header_size = 10;
	this->max_payload = 502;
	this->packet_size = this->header_size + this->max_payload;
	this->buffer = (char*)malloc(packet_size);
	this->buffer_address = (size_t)buffer;
	this->header = (packet*)malloc(header_size);
	this->header_address = (size_t)header;
	this->main_ip = main_ip;
	this->main_port = main_port;
	cout << "\n\nhost: " << this->main_ip << "\nport: " << this->main_port << "\n\n";
	//cout << "\nchamando aceita_conexoes\n";
	accept_connections();
}

void Synchronization_server::accept_connections()
{
	// If main_ip == "", this is the main server
	// Otherwise, this is a backup
	if(main_ip=="")
	{
		cout << "\n\nmain server\n\n";
	    socklen_t clilen, bkplen;
	    struct sockaddr_in serv_addr, cli_addr, serv_addr_bkp, bkp_addr;

		//-----------------------------------------------------------------------------------------
		// INITIALIZE CLIENT CONNECTION SOCKET
		//-----------------------------------------------------------------------------------------
	    // Create the socket as non-blocking. Without this, it's impossible for the server to close (since it blocks)
	    if ((client_accept_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	        printf("ERROR opening socket");
	    cout << "\nsocket aberto (porta " << port << ")\n";

	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_port = htons(port);
	    serv_addr.sin_addr.s_addr = INADDR_ANY;
	    bzero(&(serv_addr.sin_zero), 8);

	    if (bind(client_accept_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	        printf("ERROR on binding");
	    cout << "\nbinding completo\n";
	    listen(client_accept_sockfd, 5);
	    clilen = sizeof(struct sockaddr_in);
		//-----------------------------------------------------------------------------------------

		//-----------------------------------------------------------------------------------------
		// INITIALIZE BACKUP CONNECTION SOCKET
		//-----------------------------------------------------------------------------------------
		// Open the socket as non-blocking
	    if ((backup_accept_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	        printf("ERROR opening socket");
	    cout << "\nsocket aberto (porta " << backup_port << ")\n";

	    serv_addr_bkp.sin_family = AF_INET;
	    serv_addr_bkp.sin_port = htons(backup_port);
	    serv_addr_bkp.sin_addr.s_addr = INADDR_ANY;
	    bzero(&(serv_addr_bkp.sin_zero), 8);

	    if (bind(backup_accept_sockfd, (struct sockaddr *) &serv_addr_bkp, sizeof(serv_addr_bkp)) < 0)
	        printf("ERROR on binding");
	    cout << "\nbinding completo\n";
	    listen(backup_accept_sockfd, 5);
	    bkplen = sizeof(struct sockaddr_in);
		//-----------------------------------------------------------------------------------------

	    while(true)
	    {
			int newsockfd = -1;
			int backup_sockfd = -1;
			cout << "\nWaiting for connections . . .";

	        // Accept connections
	        while(newsockfd<0 && !closing_server && backup_sockfd<0){
	            // Check if any threads finished
	            check_finished_threads();
	            newsockfd = accept(client_accept_sockfd, (struct sockaddr *) &cli_addr, &clilen);
	            backup_sockfd = accept(backup_accept_sockfd, (struct sockaddr *) &bkp_addr, &bkplen);
	        }
			// CLOSING SERVER
	        if(closing_server)
	            close_server();

			// NEW BACKUP
			if(backup_sockfd>=0)
			{
		        cout << "\nNew backup connection request";
				// TODO: CRIAR THREAD PARA O BACKUP
				/*Communication_server com;
				com.Init(backup_port, header_size, max_payload);
				com.send_string(backup_sockfd, "alive");*/
			}

			// NEW CLIENT
			if(newsockfd>=0)
			{
		        cout << "\nNew client connection request";

		        // Receive username from client
		        struct packet *pkt = receive_payload(newsockfd);
		        string str_buff = pkt->_payload;
		        string new_client_username = str_buff.substr(0, pkt->length);

				// Check if the client is already connected. If it is, check if number of connections > max_connections
				// num_connections will be -1 if number of connections > max_connections. Also, if it is connected its file_server vector and its
				//file_server mutex should be the same (reference)
				vector<File_server> *files_vector = NULL;
				pthread_mutex_t *files_mutex = NULL;
				int num_connections=1;
		        for(int i=0; i<connected_clients.size(); i++){
		            //cout << "\nverificando username \"" << new_client_username << "\" contra \"" << *connected_clients[i].get_username() << "\"";
		            if(new_client_username == *connected_clients[i].get_username()){
		                num_connections = connected_clients[i].new_connection();
						files_vector = connected_clients[i].get_user_files();
						files_mutex = connected_clients[i].get_user_files_mutex();
					}
		        }

				// If this is the first client with this username or not, create the new connected client with the correct constructor
				Connected_client new_client;
				if(files_vector==NULL)
			    	new_client.init(new_client_username, newsockfd, num_connections, port, header_size, max_payload);
				else
			    	new_client.init(new_client_username, newsockfd, num_connections, port, header_size, max_payload, files_vector, files_mutex);

		        if(num_connections < 0){ // Too many connections
		            cout << "\nClient " << new_client_username << " failed to connect. Too many connections.";
		            // Tell the client to close
		            new_client.com.send_int(newsockfd, -1);
		        }
				else
				{
				    cout << "\nClient " << new_client_username << " connected";

				    // Tell the client that the connection has been accepted
				    new_client.com.send_int(newsockfd, 1);

				    // Create client folder, if it doesn't already exist
				    string homedir = getenv("HOME");
				    new_client.com.create_folder(homedir+"/server_sync_dir_"+new_client_username);

				    // add new_client to the connected_clients vector
				    connected_clients.push_back(new_client);

				    // Add an entry to the client_threads_finished vector
		            int* address = (int*)malloc(sizeof(int));
		            *address = 0;
				    threads_finished_address.push_back(address);

		            // Create a struct with the arguments to be sent to the new thread
		            struct th_args args;
		            args.obj = &new_client.com;
		            args.newsockfd = new_client.get_sockfd();
		            args.username = new_client.get_username();
		            args.thread_finished = address;
					args.user_files = new_client.get_user_files();
					args.user_files_mutex = new_client.get_user_files_mutex();

		            // Create the thread for this client
				    pthread_t client_thread;
		            pthread_create(&client_thread, NULL, new_client.com.receive_commands_helper, &args);

		            // Set the new connected client thread
		            connected_clients.back().set_thread(client_thread);
		        }
		    }
		}
	}
	//-----------------------------------------------------------------------------------------
	// Backup server
	//-----------------------------------------------------------------------------------------
	else
	{
		cout << "\n\nbackup server\n\n";

		cout << "\n\nhost: " << this->main_ip << "\nport: " << this->main_port << "\n\n";
		int sockfd = connect_backup_to_main();

		bool server_died = false;
		while(!server_died)
		{
	        // Receive heartbeat from main server
	        struct packet *pkt = receive_payload(sockfd);
			// Check if timed out
			if(pkt->type == 10)
				server_died = true;
			else
			{
		        string str_buff = pkt->_payload;
		        string msg = str_buff.substr(0, pkt->length);

				// If server closing
				if(msg=="kill")
					close_server();
				if(msg=="alive")
					cout << "\nIT'S ALIIIIIIVE!!!!!!\n";
			}
		}

		// TODO: eleição
	}
	//-----------------------------------------------------------------------------------------
}

void Synchronization_server::close_server()
{
    // Close all client sockets and join all client threads
    for(int i=0; i<connected_clients.size(); i++){
        cout << endl << "joining thread " << connected_clients[i].get_thread();
        pthread_join(connected_clients[i].get_thread(), NULL);
        cout << endl << "closing socket " << *connected_clients[i].get_sockfd();
        close(*connected_clients[i].get_sockfd());
        cout << endl << "DONE!";
		// TODO: DAR JOIN EM TODAS AS THREADS DOS BACKUPS
    }
    cout << "\nclosing accept sockets";
	close(client_accept_sockfd);
    close(backup_accept_sockfd);
    cout << "\nDONE!";
    cout << "\nserver closed\n";
    exit(0);
}

int Synchronization_server::connect_backup_to_main()
{
	struct sockaddr_in main_addr; // server_address
    struct hostent *server = gethostbyname(main_ip.c_str());

	if (server == NULL)
		std::cerr << "ERROR, no such host\n";

	cout << "\n\nhostname: " << main_ip << "\nport: " << main_port << "\n\n";

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
		std::cerr << "ERROR opening socket\n";

	main_addr.sin_family = AF_INET;
	main_addr.sin_port = htons(main_port);
	main_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(main_addr.sin_zero), 8);

	if (connect(sockfd,(struct sockaddr *) &main_addr,sizeof(main_addr)) < 0)
		std::cerr << "ERROR connecting with server\n";

	return sockfd;
}

void Synchronization_server::check_finished_threads()
{
	// Create a vector to keep track of the clients that have closed
	vector<string> closed_clients;

	// Go through the vector of connected clients, checking if any client has disconnected
    for(int i=0; i<connected_clients.size(); i++)
	{
        int *n = (int*)threads_finished_address[i];

		// If the client has disconnected, join its thread and close its socket
		// We also need to remove it from the vector of connected clients and decrement the number
		//of connections for all the clients with this username
        if(*n){
            cout << endl << "client " << *connected_clients[i].get_username() << " disconnected\n";
            cout << endl << "joining thread " << connected_clients[i].get_thread() << "......";
            pthread_join(connected_clients[i].get_thread(), NULL);
            cout << "DONE!";

            cout << endl << "closing socket " << *connected_clients[i].get_sockfd() << "......";
            close(*connected_clients[i].get_sockfd());
            cout << "DONE!" << endl;

			closed_clients.push_back(*connected_clients[i].get_username());
            connected_clients.erase(connected_clients.begin()+i);
            free(threads_finished_address[i]);
            threads_finished_address.erase(threads_finished_address.begin()+i);
        }
    }
	// Go through the connected clients again, this time decrementing the number of active connections
	//from entries with the same username as the clients that have disconnected
	for(int i=0; i<connected_clients.size(); i++)
	{
		for(int j=0; j<closed_clients.size(); j++)
		{
			if(*connected_clients[i].get_username() == closed_clients[j])
				connected_clients[i].remove_connection();
		}
	}
}

packet* Synchronization_server::receive_header(int sockfd)
{
    buffer = (char*)buffer_address;
    header = (packet*)header_address;

	int bytes_received=0;
	bool timedout = false;
	clock_t start = clock();
    while(bytes_received < header_size && !timedout)
    {
		// TIMEOUT!
		if(((clock()-start) / (double)CLOCKS_PER_SEC) >= 10)
			timedout = true;

        int n = read(sockfd, &buffer[bytes_received], header_size-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");

        bytes_received+=n;
	}
	if(timedout){
		header->type = 10;
		return header;
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
	// Check if timed out
	if(pkt->type == 10)
		return pkt;

	int bytes_received=0;
	bool timedout = false;
	clock_t start = clock();
    while(bytes_received < pkt->length && !timedout)
    {
		// TIMEOUT!
		if(((clock()-start) / (double)CLOCKS_PER_SEC) >= 10)
			timedout = true;

        // read from the socket
        int n = read(sockfd, &buffer[bytes_received], pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");

        bytes_received+=n;
	}
	pkt->_payload = (const char*)buffer;
	return pkt;
}
