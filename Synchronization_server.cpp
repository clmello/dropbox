#include "Synchronization_server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// This global variable tells all the threads in the server that the server will close
extern bool closing_server;


void Synchronization_server::Init(int port)
{
	this->port = port;
	this->backup_port = port+1;
	this->chk_port = port+2;
	this->header_size = 10;
	this->max_payload = 502;
	this->packet_size = this->header_size + this->max_payload;
	this->buffer = (char*)malloc(packet_size);
	this->buffer_address = (size_t)buffer;
	this->header = (packet*)malloc(header_size);
	this->header_address = (size_t)header;

	accept_connections();
}

void Synchronization_server::accept_connections()
{
	vector<int> r_w_backups;
	vector<pthread_mutex_t> backup_mutexes;
	r_w_backups.push_back(0);
	r_w_backups.push_back(0);
	pthread_mutex_t r_w_backups_mutex;
	pthread_mutex_init(&r_w_backups_mutex, NULL);
	vector<int> backup_sockets;
	vector<std::string> backup_ips;
	vector<pthread_t> chk_threads;

    socklen_t clilen, bkplen, chklen;
    struct sockaddr_in serv_addr, cli_addr, serv_addr_bkp, bkp_addr, serv_addr_chk, chk_addr;

	//-----------------------------------------------------------------------------------------
	// INITIALIZE CLIENT CONNECTION SOCKET
	//-----------------------------------------------------------------------------------------
    // Create the socket as non-blocking. Without this, it's impossible for the server to close (since it blocks)
    if ((client_accept_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
        printf("ERROR opening socket");
    cout << "\nClient socket open (port " << port << ")";

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    if (bind(client_accept_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("ERROR on binding");
    listen(client_accept_sockfd, 5);
    clilen = sizeof(struct sockaddr_in);
	//-----------------------------------------------------------------------------------------

	//-----------------------------------------------------------------------------------------
	// INITIALIZE BACKUP CONNECTION SOCKET
	//-----------------------------------------------------------------------------------------
	// Open the socket as non-blocking
    if ((backup_accept_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
        printf("ERROR opening socket");
    cout << "\nBackup socket open (port " << backup_port << ")";

    serv_addr_bkp.sin_family = AF_INET;
    serv_addr_bkp.sin_port = htons(backup_port);
    serv_addr_bkp.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr_bkp.sin_zero), 8);

    if (bind(backup_accept_sockfd, (struct sockaddr *) &serv_addr_bkp, sizeof(serv_addr_bkp)) < 0)
        printf("ERROR on binding");
    listen(backup_accept_sockfd, 5);
    bkplen = sizeof(struct sockaddr_in);
	//-----------------------------------------------------------------------------------------

	//-----------------------------------------------------------------------------------------
	// INITIALIZE CHECK_SERVER CONNECTION SOCKET
	//-----------------------------------------------------------------------------------------
	// Open the socket as non-blocking
	if ((chk_accept_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	printf("ERROR opening socket");
	cout << "\nCheck_server socket open (port " << chk_port << ")\n";

	serv_addr_chk.sin_family = AF_INET;
	serv_addr_chk.sin_port = htons(chk_port);
	serv_addr_chk.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr_chk.sin_zero), 8);

	if (bind(chk_accept_sockfd, (struct sockaddr *) &serv_addr_chk, sizeof(serv_addr_chk)) < 0)
		printf("ERROR on binding");
	listen(chk_accept_sockfd, 5);
	chklen = sizeof(struct sockaddr_in);
	//-----------------------------------------------------------------------------------------

    while(true)
    {
		int newsockfd = -1;
		int backup_sockfd = -1;
		int chk_sockfd = -1;
		cout << endl << "Waiting for connections . . .";
		cout << endl;

        // Accept connections
		int time_between_signals = 3;
		clock_t start = clock();
		// Signal a first time so we don't have to wait 3 seconds
		signal_alive();
        while(newsockfd<0 && !closing_server && backup_sockfd<0 && chk_sockfd<0)
		{
            // Check if any threads finished
            check_finished_threads();

			clock_t delta = clock()-start;
			int delta_sec = delta / CLOCKS_PER_SEC;
			if(delta_sec >= time_between_signals){
				signal_alive();
				start = clock();
			}

            newsockfd = accept(client_accept_sockfd, (struct sockaddr *) &cli_addr, &clilen);
            backup_sockfd = accept(backup_accept_sockfd, (struct sockaddr *) &bkp_addr, &bkplen);
            chk_sockfd = accept(chk_accept_sockfd, (struct sockaddr *) &chk_addr, &chklen);
        }
		// CLOSING SERVER
        if(closing_server)
            close_server();

		// NEW CHECK_SERVER
		else if(chk_sockfd>=0)
		{
			cout << endl << "New check_server connection request";
			chk_sockets.push_back(chk_sockfd);
            // Create the thread for check_server
			/*Communication_server com;
			com.Init(chk_port, header_size, max_payload);
			struct chk_alive_args args_chk;
			args_chk.obj = &com;
			args_chk.sockfd = &chk_sockfd;

		    pthread_t chk_thread;
            pthread_create(&chk_thread, NULL, com.signal_server_alive_helper, &args_chk);
			chk_threads.push_back(chk_thread);*/
		}

		// NEW BACKUP
		else if(backup_sockfd>=0)
		{
			cout << endl << "New backup connection request";
			while(true)
			{
				pthread_mutex_lock(&r_w_backups_mutex);
				// If no one writing or reading, continue
				if(r_w_backups[1]==0 && r_w_backups[0]==0)
					break;
				pthread_mutex_unlock(&r_w_backups_mutex);
			}
			// Writing = true
			r_w_backups[1]++;
			pthread_mutex_unlock(&r_w_backups_mutex);

			std::string s = inet_ntoa(bkp_addr.sin_addr);
			std::cout << "\nIP BACKUP: " << s;
			backup_ips.push_back(s);
			std::cout << "\nSOCKET BACKUP: " << backup_sockfd;
			backup_sockets.push_back(backup_sockfd);
			/*
			struct sockaddr_in *addr_in = (struct sockaddr_in *)res;
			char *s = inet_ntoa(addr_in->sin_addr);
			printf("IP address: %s\n", s);
			 */
			pthread_mutex_t mtx;
			backup_mutexes.push_back(mtx);
			pthread_mutex_init(&backup_mutexes.back(), NULL);

			// Stop writing
			pthread_mutex_lock(&r_w_backups_mutex);
			r_w_backups[1]--;
			pthread_mutex_unlock(&r_w_backups_mutex);

			// Send all server files to the backup
			send_all_files(backup_sockfd);
		}

		// NEW CLIENT
		else if(newsockfd>=0)
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

				// Send backups IPs and sockets
				int size = backup_ips.size();
				new_client.com.send_int(newsockfd, size);

				// backup_sockets and backup_ips should be the same size
				for(int i=0; i < backup_ips.size(); i++) {
					new_client.com.send_string(newsockfd, backup_ips[i]);
				}

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
				args.backup_sockets = &backup_sockets;
				args.backup_mutexes = &backup_mutexes;
				args.r_w_backups_mutex = &r_w_backups_mutex;
				args.r_w_backups = &r_w_backups;


	            // Create the thread for this client
			    pthread_t client_thread;
	            pthread_create(&client_thread, NULL, new_client.com.receive_commands_helper, &args);

	            // Set the new connected client thread
	            connected_clients.back().set_thread(client_thread);
	        }
	    }
	}
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

void Synchronization_server::signal_alive()
{
	Communication_server com;
	com.Init(chk_port, header_size, max_payload);
	for(int i=0; i<chk_sockets.size(); i++)
	{
		//cout << endl << "sending alive to " << chk_sockets[i];
		com.send_string(chk_sockets[i], "alive");
	}
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

int Synchronization_server::get_num_users()
{
	vector<string> visited_usernames;
	int num_users = 0;
	for(int i=0; i<connected_clients.size(); i++)
	{
		// Only increment num_users if the username has not been visited
		string username = *connected_clients[i].get_username();
		if(find(visited_usernames.begin(), visited_usernames.end(), username) == visited_usernames.end())
		{
			visited_usernames.push_back(*connected_clients[i].get_username());
			num_users ++;
		}
	}

	return num_users;
}

int Synchronization_server::get_num_files(string username)
{
	int num_files = 0;
	vector<File_server> *user_files;

	for(int index=0; index<connected_clients.size(); index++)
	{
		if(username == *connected_clients[index].get_username()){
			user_files = connected_clients[index].get_user_files();
			break;
		}
	}
	cout << endl << "User: " << username;
	cout << endl << "Existem " << user_files->size() << " arquivos";
	for(int i=0; i<user_files->size(); i++)
	{
		cout << endl << "The file " << (*user_files)[i].get_filename() << " has mtime == " << (*user_files)[i].get_mtime();
		if((*user_files)[i].get_mtime()!=-1)
			num_files++;
	}
	return num_files;
}

void Synchronization_server::send_all_files(int sockfd)
{
	// Lock all the user_files mutexes so that no new file is added while we send the files to the backup
	for(int i=0; i<connected_clients.size(); i++)
		pthread_mutex_lock(connected_clients[i].get_user_files_mutex());

	vector<string> visited_usernames;
	Communication_server com;
	com.Init(backup_port, header_size, max_payload);
	// Send the number of connected USERS
	com.send_int(sockfd, get_num_users());
	cout << endl << "I HAVE " << get_num_users() << " USERS";
	for(int i=0; i<connected_clients.size(); i++)
	{
		// Only send the files if the username has not been visited
		string username = *connected_clients[i].get_username();
		if(find(visited_usernames.begin(), visited_usernames.end(), username) == visited_usernames.end())
		{
			visited_usernames.push_back(*connected_clients[i].get_username());
			// Send the username
			com.send_string(sockfd, username);
			// Send the number of files
			com.send_int(sockfd, get_num_files(username));
			cout << endl << username << " HAS " << connected_clients[i].get_user_files_size() << " FILES";
			if(connected_clients[i].get_user_files_size()>0)
			{
				for(int j=0; j<connected_clients[i].get_user_files()->size(); j++)
				{
					if((*connected_clients[i].get_user_files())[j].get_mtime() != -1)
					{
						// Send each filename
						com.send_string(sockfd, (*connected_clients[i].get_user_files())[j].get_filename());
						// Send each file
						com.send_file(sockfd, (*connected_clients[i].get_user_files())[j].get_path());
					}
				}
			}
		}
		// If it has been visited, tell the backup that there are 0 files
		else
			com.send_int(sockfd, 0);
	}

	// Unlock all the user_files mutexes
	for(int i=0; i<connected_clients.size(); i++)
		pthread_mutex_unlock(connected_clients[i].get_user_files_mutex());
}

packet* Synchronization_server::receive_header(int sockfd)
{
    buffer = (char*)buffer_address;
    header = (packet*)header_address;

	int bytes_received=0;

    while(bytes_received < header_size)
    {
        int n = read(sockfd, &buffer[bytes_received], header_size-bytes_received);
		if(n > 0)
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
	// Check if timed out
	if(pkt->type == 10)
		return pkt;

	int bytes_received=0;

    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, &buffer[bytes_received], pkt->length-bytes_received);
		if(n > 0)
        	bytes_received+=n;
	}
	pkt->_payload = (const char*)buffer;
	return pkt;
}
