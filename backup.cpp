#include "backup.h"

using namespace std;

Backup::Backup(string main_ip, int main_port)
{
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
	main_sockfd = connect_backup_to_main();
	chk_sockfd = connect_chk_server();

	int server_died = false;

	// Create check_server thread
	struct bkp_args args;
	args.obj = this;
	args.main_check_sockfd = &chk_sockfd;
	args.server_died = &server_died;
    pthread_create(&chk_thread, NULL, &check_server_helper, &args);

	// Receive commands loop
	receive_commands(main_sockfd, &server_died);
	pthread_join(chk_thread, NULL);

	// TODO: eleição

	exit(0);
}

void *Backup::check_server_helper(void* void_args)
{
    bkp_args* args = (bkp_args*)void_args;
    ((Backup*)args->obj)->check_server(args->main_check_sockfd, args->server_died);
    return 0;
}

void Backup::check_server(int* main_check_sockfd, int *server_died)
{
	while(!*server_died)
	{
		// Receive heartbeat from main server
		struct packet *pkt = receive_payload(*main_check_sockfd, 10);
		// Check if timed out
		if(pkt->type == 10){
			*server_died = true;
			exit(0);
		}
		else
		{
			string str_buff = pkt->_payload;
			string msg = str_buff.substr(0, pkt->length);
			cout << endl << "msg: " << msg;
			cout << endl;
			// If server closing
			if(msg=="kill")
				close_backup(*main_check_sockfd);
			else if(msg=="alive")
				cout << endl << "IT'S ALIIIIIIVE!!!!!!" << endl;
		}
	}
}

void Backup::close_backup(int main_check_sockfd)
{
	close(main_sockfd);
	close(main_check_sockfd);
	exit(0);
}

void Backup::receive_commands(int sockfd, int *server_died)
{
	bool server_closing = false;
	while(!server_closing && !*server_died)
	{
		cout << endl << sockfd << ": waiting for command";
		int command = receive_int(sockfd);
		switch(command)
		{
			case 1: // Upload to backup
			{
				cout << endl << sockfd << ": receiving file";

                // Receive username
		        struct packet *pkt;
				pkt = receive_payload(sockfd, 0);
                string username = pkt->_payload;
				username.resize(pkt->length);

				// Create user folder (if it does not exist)
				string path = create_user_folder(username);

                // Receive the file name
                pkt = receive_payload(sockfd, 0);
                string filename = pkt->_payload;
				filename.resize(pkt->length);

				// Receive file
				receive_file(sockfd, (path+"/"+filename));

				break;
			}
			case 2: // Delete from backup
			{
				cout << endl << sockfd << ": deleting file";

                // Receive username
		        struct packet *pkt;
				pkt = receive_payload(sockfd, 0);
                string username = pkt->_payload;
				username.resize(pkt->length);

                // Receive the file name
                pkt = receive_payload(sockfd, 0);
                string filename = pkt->_payload;
				filename.resize(pkt->length);

			    const char *homedir = getenv("HOME");
			    string syncdir = "/bkp_sync_dir_" + username;
			    string path = string(homedir) + syncdir + "/" + filename;
				remove(path.c_str());

				break;
			}
		}
	}
}

string Backup::create_user_folder(string username)
{
    const char *homedir = getenv("HOME");
    string syncdir = "/bkp_sync_dir_" + username;
    string path = string(homedir) + syncdir;

	DIR* dir = opendir(path.c_str());
    if(!dir)
    {
        string command = "mkdir -p " + path;
        int error = system(command.c_str());
        if(error < 0)
            cout << endl << "Error creating folder";
    }
    closedir(dir);
	return path;
}

int Backup::connect_backup_to_main()
{
	struct sockaddr_in main_addr; // server_address
    struct hostent *server = gethostbyname(main_ip.c_str());

	if (server == NULL)
		std::cerr << "ERROR, no such host\n";

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
		std::cerr << "ERROR opening socket\n";

	main_addr.sin_family = AF_INET;
	main_addr.sin_port = htons(main_port);
	main_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(main_addr.sin_zero), 8);

	if (connect(sockfd,(struct sockaddr *) &main_addr,sizeof(main_addr)) < 0)
		std::cerr << "ERROR connecting with server\n";

	cout << endl << "Backup connected";
	return sockfd;
}

int Backup::connect_chk_server()
{
	struct sockaddr_in main_addr; // server_address
    struct hostent *server = gethostbyname(main_ip.c_str());

	if (server == NULL)
		std::cerr << "ERROR, no such host\n";

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
		std::cerr << "ERROR opening socket\n";

	main_addr.sin_family = AF_INET;
	main_addr.sin_port = htons(main_port+1);
	main_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(main_addr.sin_zero), 8);

	if (connect(sockfd,(struct sockaddr *) &main_addr,sizeof(main_addr)) < 0)
		std::cerr << "ERROR connecting with server\n";

	// Make socket non-blocking
	int fl = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, fl | O_NONBLOCK);

	cout << endl << "Check_server connected";
	return sockfd;
}

int Backup::receive_int(int sockfd)
{
	struct packet *pkt = receive_payload(sockfd, 0);
	int command;
	memcpy(&command, pkt->_payload, pkt->length);
	return command;
}

packet* Backup::receive_header(int sockfd, int timeout_sec)
{
	cout << endl << "entrei no receive_header";
	cout << endl;
    buffer = (char*)buffer_address;
    header = (packet*)header_address;

	int bytes_received=0;
	bool timedout = false;
	struct timeval timeout;

	if(timeout_sec > 0)
	{
		// Set up the timeout
		fd_set input;
		FD_ZERO(&input);
		FD_SET(sockfd, &input);
		timeout.tv_sec = timeout_sec;
		timeout.tv_usec = 0;
		int n = select(sockfd + 1, &input, NULL, NULL, &timeout);
		if(n<0)
			cout << endl << "Select error";
	}

    while(bytes_received < header_size && !timedout)
    {
        int n = read(sockfd, &buffer[bytes_received], header_size-bytes_received);
		if(n > 0)
        	bytes_received+=n;
		else if(n <= 0 && timeout_sec>0)
			timedout = true;
	}
	if(timedout){
		cout << endl << "TIMEOUT NO RECEIVE_HEADER!";
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

packet* Backup::receive_payload(int sockfd, int timeout_sec)
{
    struct packet *pkt = receive_header(sockfd, timeout_sec);
	// Check if timed out
	if(pkt->type == 10)
		return pkt;

	int bytes_received=0;
	bool timedout = false;
	struct timeval timeout;

	if(timeout_sec > 0)
	{
		// Set up the timeout
		fd_set input;
		FD_ZERO(&input);
		FD_SET(sockfd, &input);
		timeout.tv_sec = timeout_sec;
		timeout.tv_usec = 0;
		int n = select(sockfd + 1, &input, NULL, NULL, &timeout);
	}

    while(bytes_received < pkt->length && !timedout)
    {
        // read from the socket
        int n = read(sockfd, &buffer[bytes_received], pkt->length-bytes_received);
		if(n > 0)
        	bytes_received+=n;
		else if(n <= 0 && timeout_sec>0)
			timedout = true;
	}
	pkt->_payload = (const char*)buffer;
	if(timedout){
		cout << endl << "TIMEOUT NO RECEIVE_HEADER!";
		header->type = 10;
	}
	return pkt;
}

void Backup::receive_file(int sockfd, string path)
{
    FILE *fp = fopen(path.c_str(), "w");
    if(fp==NULL)
        cout << "\nERROR OPENING " << path << endl;

    // Get the number of packets to be received
    // To do that, we must receive the first packet
    struct packet *pkt;
    pkt = receive_payload(sockfd, 0);
    uint32_t total_size = pkt->total_size;

    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
    if (bytes_written_to_file < pkt->length)
        cout << "\nERROR WRITING TO " << path << endl;


    // Receive all the [total_size] packets
    // It starts at 2 because the first packet has already been received
    int i;
    for(i=2; i<=total_size; i++)
    {
        // Receive payload
        receive_payload(sockfd, 0);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
        if (bytes_written_to_file < pkt->length)
            cout << "\nERROR WRITING TO " << path << endl;
    }
    fclose(fp);
}
