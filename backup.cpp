#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "backup.h"

using namespace std;

vector<backup_info> backups_list;

Backup::Backup(string main_ip, int main_port, int backup_port)
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
	this->backup_port = backup_port;

	bool is_main = false;
	connected = false;
	int server_died = false;
	leader = false; // ṕor enquanto esse backup não é o lider
	backup_id = 0;

	struct backup_info bkp_info;
	struct backup_info this_backup;

	while(!is_main)
	{
		cout << "\n\nhost: " << this->main_ip << "\nport: " << this->main_port << "\n\n";
		while(!connected){
			main_sockfd = connect_backup_to_main();
			// If could not connect, sleep for 3 seconds so the requests won't flood the network
			if(!connected)
				sleep(3);
		}

		// recebe numero de backups pra se conectar
		int num_backups = receive_int(main_sockfd, 30);
		std::cout << "!!!!!!!!!!!!!!!!!!!!! VAI TESTAR RECEBIMENTO DE IPS\n";
		cout << endl << "existem " << num_backups << " backups";
		// se >= 1 -> tenta se conectar com os ips recebidos e depois abre thread pra esperar conexão
		if(num_backups > 0) {
			for(int i=0; i < num_backups; i++) {
				struct packet *pkt = receive_payload(main_sockfd, 10);
				std::string ip = pkt->_payload;
				std::cout << "!!!!!!!BACKUP IP: " << ip << "\n";
				int c = connect_backup_to_backup(ip);
				if(c > 0 ){
					std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
					std::cout << "!!!!!!!!!!!!!!!!! BACKUP CONNECTED !!!!!!!!!!!!!!!!!!\n";
				}
				backup_id++;
    		}
		} else
			std::cout << "No backup on list\n";

		// o backup guarda seu socket como -1, porque ai tu sabe qual o id dele (o lugar dele na lista)

		//bkp_info.ip = "";
		//bkp_info.sockfd = -1;
		//bkp_info.id = backup_id;
		//backups.push_back(bkp_info);
		this_backup.ip = "";
		this_backup.sockfd = -1;
		this_backup.id = backup_id;
		backup_id++;
		struct bkp_args backup_args;
		backup_args.obj = this;
		backup_args.main_check_sockfd = &chk_sockfd;
		backup_args.server_died = &server_died;
		// se 0 -> só abre thread pra esperar por conexões
		std::cout << "Vai abrir a thread pra esperar conexões\n";
		pthread_create(&connect_backups_thread, NULL, &connect_backups_helper, &backup_args);
		std::cout << "A thread funcionou\n";

		receive_server_files(main_sockfd);
		chk_sockfd = connect_chk_server();

		// Create check_server thread
		struct bkp_args args;
		args.obj = this;
		args.main_check_sockfd = &chk_sockfd;
		args.server_died = &server_died;
	    pthread_create(&chk_thread, NULL, &check_server_helper, &args);

		// Receive commands loop
		receive_commands(main_sockfd, &server_died);
		pthread_join(chk_thread, NULL);
		pthread_join(connect_backups_thread, NULL);

		close(main_sockfd);
		close(chk_sockfd);

		cout << endl << endl << "Electing new main server" << endl;
		// TODO: eleição
		// A função election() deve retornar "" para o novo main server e "IP_do_novo_main"
		//para todos os outros
		string new_host = election(this_backup);
		/*int leader = election(this_backup);
		cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
		cout << "\n!!!!!!!!!!!!!!!!!!!!!! LEADER: " << leader;
		cout << endl;
		exit(0);*/
		//----------------------------
		// Essa linha é só p/ testes
		//string new_host = "";
		//----------------------------
		if(new_host=="")
			is_main = true;
		else
		{
			main_ip = new_host;
			this->main_port = main_port + 3;
			connected = false;
			cout << endl << "New main port: " << main_port;
		}
	}

	// If the code gets here, this backup has been elected the new main server
	Synchronization_server sync_server;
	// If the ports were 4000, 4001 and 4002, they become 4003, 4004 and 4005
	sync_server.Init(main_port+2);

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
			cout << endl << "SERVER DIED!!";
			cout << endl;
			*server_died = true;
		}
		else
		{
			string str_buff = pkt->_payload;
			string msg = str_buff.substr(0, pkt->length);
			/*cout << endl << "msg: " << msg;
			cout << endl;*/
			// If server closing
			if(msg=="kill")
				close_backup(*main_check_sockfd);
			/*else if(msg=="alive")
				cout << endl << "IT'S ALIIIIIIVE!!!!!!" << endl;*/
		}
	}
	cout << endl << "saiu do while do check_server" << endl;
}

void Backup::close_backup(int main_check_sockfd)
{
	close(main_sockfd);
	close(main_check_sockfd);
	exit(0);
}

void *Backup::connect_backups_helper(void* void_args)
{
    bkp_args* args = (bkp_args*)void_args;
    ((Backup*)args->obj)->connect_backup(args->main_check_sockfd, args->server_died);
    return 0;
}

void Backup::connect_backup(int* main_check_sockfd, int *server_died) {
	// 1. inicializa o socket da conexão com backup
	// 2. fica esperando por uma conexão de outro backup com id maior (ex: backup de id 0 que o backup de id 1 se conecte)
	// 3. depois de se conectar, adiciona o socket na lista de sockets
	struct backup_info bkp_info;
	struct sockaddr_in bkp_addr_bkp, bkp_addr;
	socklen_t bkplen;

	// INITIALIZE BACKUP CONNECTION SOCKET
	// Open the socket as non-blocking
	if ((bkp_accept_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	    printf("ERROR opening socket");
	cout << "\nBackup socket open (port " << backup_port << ")";

	bkp_addr_bkp.sin_family = AF_INET;
	bkp_addr_bkp.sin_port = htons(backup_port);
	bkp_addr_bkp.sin_addr.s_addr = INADDR_ANY;
	bzero(&(bkp_addr_bkp.sin_zero), 8);

	if (bind(bkp_accept_sockfd, (struct sockaddr *) &bkp_addr_bkp, sizeof(bkp_addr_bkp)) < 0)
	    printf("ERROR on binding");
	listen(bkp_accept_sockfd, 5);
	bkplen = sizeof(struct sockaddr_in);

	std::cout << "Esperando conexão\n";
	while(!*server_died) {
		int backup_sockfd = -1;
		backup_sockfd = accept(bkp_accept_sockfd, (struct sockaddr *) &bkp_addr, &bkplen);
		if(backup_sockfd > 0) {
			std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
			std::cout << "!!!!!!!!!!!!!!!!!!! CONECTOU !!!!!!!!!!!!!!!!!!!!!!!!\n";
			// add sockfd to list

			std::string ip = inet_ntoa(bkp_addr.sin_addr);
			bkp_info.ip = ip;
			bkp_info.sockfd = backup_sockfd;
			bkp_info.id = backup_id;
			backups_list.push_back(bkp_info);

			for(int i; i < backups_list.size(); i++) {
				cout << "\nbackup ip:" << backups_list[i].ip;
				cout << "\nbackup sockfd:" << backups_list[i].sockfd;
				cout << "\nbackup id:" << backups_list[i].id;

			}
			backup_id++;
		}
	}
	std::cout << "Thread funcionando \n";
}

string Backup::election(struct backup_info this_backup) {
	struct packet *pkt;
	Communication_server com;
	com.Init(backup_port, 10, 502);
	vector<int> received_ids;
	int leader_id;

	leader = true;
	leader_id = this_backup.id;

	// se for o menor id, manda a mensagem eleição
	if(this_backup.id == 0) {
		cout << endl << "this is the first backup\nsending election";
		// Send election to all backups with higher ID and wait for the answers
		cout << endl << backups_list.size() << " backups exist";
		cout << endl << "my ID is " << this_backup.id;
		for(int i = this_backup.id + 1; i < backups_list.size(); i++) {
			com.send_int(backups_list[i].sockfd, this_backup.id);
			cout << endl << "sent election to " << backups_list[i].id;
			int id = receive_int(backups_list[i].sockfd, this_backup.id * 10);
			cout << endl << "received answer from " << backups_list[i].id;
			received_ids.push_back(id);
		}

		// os ids que tem -10 é porque deu timeout
		// If there is any other active backup, it will be the leader
		cout << endl << "checking if any higer ID answered";
		for(int i = 0; i < received_ids.size(); i++) {
			cout << endl << endl << "i: " << i;
			cout << endl << "id: " << received_ids[i];
			//TODO: achar o maior
			if(received_ids[i] != -10) {
				leader = false;
				leader_id = received_ids[i];
			}
		}
	} else { // Se não for o backup de menor id, fica esperando por uma mensagem
		cout << endl << "NOT the first backup\nreceiving election";
		cout << endl;
		// Wait for election from smaller IDs and then send answer
		for(int i = 0; i < this_backup.id; i++) {
			int id = receive_int(backups_list[i].sockfd, this_backup.id * 10);
			cout << endl << "received election from " << id;
			com.send_int(backups_list[i].sockfd, backups_list[i].id);
			cout << endl << "sending answer";
			received_ids.push_back(id);
		}

			cout << endl << "sending election/receiving answers";
			cout << endl;
		// Send election to higher IDs and wait for answer
		cout << endl << backups_list.size() << " backups exist";
		cout << endl << "my ID is " << this_backup.id;
		for(int i = this_backup.id + 1; i < backups_list.size(); i++) {
			cout << endl << "sending election to " << backups_list[i].id;
			com.send_int(backups_list[i].sockfd, backups_list[i].id);
			cout << endl << "waiting for answer";
			int id = receive_int(backups_list[i].sockfd, this_backup.id * 10);
		}

		cout << endl << "looking for higher IDs";
		// If any higher ID answered, then the highest one is the new main
		leader_id = this_backup.id;
		for(int i = 0; i < received_ids.size(); i++) {
			leader = true;
			cout << endl << endl << "checking ID: " << received_ids[i];
			cout << endl << "against my ID: " << this_backup.id;
			if(received_ids[i] > leader_id) {
				leader = false;
				leader_id = received_ids[i];
			}
		}

	}

	cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	cout << "\n!!!!!!!!!!!!!!!!!!!!!! LEADER: " << leader_id;
	cout << endl;

	// Find the IP of the leader, and return it
	for(int i=0; i<backups_list.size(); i++)
	{
		if(backups_list[i].id == leader_id)
			return backups_list[i].ip;
	}

	// If this is the leader, return an empty string
	return "";

}

void Backup::receive_commands(int sockfd, int *server_died)
{
	bool server_closing = false;
	while(!server_closing && !*server_died)
	{
		cout << endl << sockfd << ": waiting for command";
		int command = receive_int(sockfd, 30);
		switch(command)
		{
			case 1: // Upload to backup
			{
				cout << endl << sockfd << ": receiving file";

                // Receive username
		        struct packet *pkt;
				pkt = receive_payload(sockfd, 30);
                string username = pkt->_payload;
				username.resize(pkt->length);

				// Create user folder (if it does not exist)
				string path = create_user_folder(username);

                // Receive the file name
                pkt = receive_payload(sockfd, 30);
                string filename = pkt->_payload;
				filename.resize(pkt->length);

				// TODO: Receive mtime
                pkt = receive_payload(sockfd, 30);
				time_t mtime= *(time_t*)pkt->_payload;

				// Receive file
				receive_file(sockfd, (path+"/"+filename));

				mtime_to_file((path+"/"+filename), mtime, username);

				break;
			}
			case 2: // Delete from backup
			{
				cout << endl << sockfd << ": deleting file";

                // Receive username
		        struct packet *pkt;
				pkt = receive_payload(sockfd, 30);
                string username = pkt->_payload;
				username.resize(pkt->length);

                // Receive the file name
                pkt = receive_payload(sockfd, 30);
                string filename = pkt->_payload;
				filename.resize(pkt->length);

			    const char *homedir = getenv("HOME");
			    string syncdir = "/bkp_sync_dir_" + username;
			    string path = string(homedir) + syncdir + "/" + filename;
				remove(path.c_str());

				delete_mtime_from_file(path, username);

				break;
			}
		}
	}
	cout << endl << "Saiu do while do receive_commands";
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
	cout << endl << endl << "Tentando conectar com " << main_ip << ":" << main_port << endl << endl;
	struct sockaddr_in main_addr; // server_address
    struct hostent *server = gethostbyname(main_ip.c_str());
	int sockfd;

	if (server == NULL)
		std::cerr << "ERROR, no such host\n";
	else
	{
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockfd == -1)
			std::cerr << "ERROR opening socket\n";
		else
		{
			main_addr.sin_family = AF_INET;
			main_addr.sin_port = htons(main_port);
			main_addr.sin_addr = *((struct in_addr *)server->h_addr);
			bzero(&(main_addr.sin_zero), 8);

			if (connect(sockfd,(struct sockaddr *) &main_addr,sizeof(main_addr)) < 0)
				std::cerr << "ERROR connecting with server\n";
			else
				connected = true;
			// Make socket non-blocking
			int fl = fcntl(sockfd, F_GETFL, 0);
			fcntl(sockfd, F_SETFL, fl | O_NONBLOCK);
		}
	}
	return sockfd;
}

int Backup::connect_backup_to_backup(string ip) {
	int sockfd;
	struct backup_info bkp_info;
	cout << endl << endl << "Tentando conectar com " << ip << ":" << backup_port << endl << endl;
	struct sockaddr_in other_backup_addr;
    struct hostent *other_backup = gethostbyname(ip.c_str());

	if (other_backup == NULL)
		std::cerr << "ERROR, no such host\n";
	else
	{
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockfd == -1)
			std::cerr << "ERROR opening socket\n";
		else
		{
			other_backup_addr.sin_family = AF_INET;
			other_backup_addr.sin_port = htons(backup_port);
			other_backup_addr.sin_addr = *((struct in_addr *)other_backup->h_addr);
			// acho que ainda é 8??
			bzero(&(other_backup_addr.sin_zero), 8);

			if (connect(sockfd,(struct sockaddr *) &other_backup_addr,sizeof(other_backup_addr)) < 0)
				std::cerr << "ERROR connecting with backup\n";
			else
				connected = true;
			// Make socket non-blocking
			int fl = fcntl(sockfd, F_GETFL, 0);
			fcntl(sockfd, F_SETFL, fl | O_NONBLOCK);

			bkp_info.ip = ip;
			bkp_info.sockfd = sockfd;
			bkp_info.id = backup_id;
			backups_list.push_back(bkp_info);
		}
	}

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

int Backup::receive_int(int sockfd, int timeout)
{
	struct packet *pkt = receive_payload(sockfd, timeout);
	if(pkt->type == 10) {
		return -10;
	}
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
    pkt = receive_payload(sockfd, 30);
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
        receive_payload(sockfd, 30);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
        if (bytes_written_to_file < pkt->length)
            cout << "\nERROR WRITING TO " << path << endl;
    }
    fclose(fp);
}

void Backup::receive_server_files(int sockfd)
{
    const char *homedir = getenv("HOME");
    string syncdir = "/bkp_sync_dir_";
    string base_path = string(homedir) + syncdir;

	// Receive the number of clients
	int num_clients = receive_int(sockfd, 30);
	cout << endl << "The server has " << num_clients << " clients";
	for(int i=0; i<num_clients; i++)
	{
		// Receive the username
		struct packet *pkt = receive_payload(sockfd, 30);
		string str_buff = pkt->_payload;
		string username = str_buff.substr(0, pkt->length);
		// Create the user folder, if it doesn't already exist
		create_user_folder(username);
		// Receive the number of files
		int num_files = receive_int(sockfd, 30);
		cout << endl << "The user has " << num_files << " files";
		for(int j=0; j<num_files; j++)
		{
			// Receive file name
			pkt = receive_payload(sockfd, 30);
			str_buff = pkt->_payload;
			string filename = str_buff.substr(0, pkt->length);

			string path = base_path + username + "/" + filename;

			// Receive file
			receive_file(sockfd, path);
		}
	}
}

void Backup::mtime_to_file(string path, time_t mtime, string username)
{
	// Set txt path
	string path_txt = getenv("HOME");
	path_txt += "/bkp_sync_dir_" + username + "/" + "mtimes";

	ifstream ifile;
	ifile.open(path_txt.c_str());
	string buff;
	vector <string> content;
	bool found = false;

	bool exists = ifile.good();

	// Read whole file
	for(int i; ifile >> buff && exists; i++)
	{
		// save name
		content.push_back(buff);
		if(buff == path)
			found = true;
		content.push_back(" ");
		// save mtime
		ifile >> buff;
		content.push_back(buff);
		content.push_back("\n");
	}

	// If there is no entry for the path
	if(!found){
		ifile.close();
		ofstream ofile;
		ofile.open(path_txt.c_str(), std::fstream::app);
		// Append info to the end of file
		ofile << path << " " << mtime << "\n";
		ofile.close();
	}
	// If there is an entry, edit it
	else{
		ifile.close();
		ofstream ofile;
		ofile.open(path_txt.c_str(), std::fstream::trunc);
		// Rewrite whole file
		for(int i=0; i<content.size(); i++)
		{
			// path
			ofile << content[i];
			// Replace the mtime for the path
			if(content[i] == path)
				content[i+2] = mtime;
			i++;
			// " "
			ofile << content[i];
			i++;
			// mtime
			ofile << content[i];
			i++;
			// "\n"
			if(i<content.size())
				ofile << content[i];
		}
		ofile.close();
	}
}

void Backup::delete_mtime_from_file(string path, string username)
{
	cout << endl << "ENTREI NO DELETE_MTIME";
	cout << endl;
	// Set txt path
	string path_txt = getenv("HOME");
	path_txt += "/bkp_sync_dir_" + username + "/" + "mtimes";

	ifstream ifile;
	ifile.open(path_txt.c_str());
	string buff;
	vector <string> content;
	//bool found = false;

	//bool exists = ifile.good();

	// Read whole file
	for(int i; ifile >> buff; i++)
	{
		// save name
		content.push_back(buff);
		//if(buff == path)
		//	found = true;
		content.push_back(" ");
		// save mtime
		ifile >> buff;
		content.push_back(buff);
		content.push_back("\n");
	}

	// Remove entry
	ifile.close();
	ofstream ofile;
	ofile.open(path_txt.c_str(), std::fstream::trunc);
	// Rewrite whole file
	int bytes_written=0;
	for(int i=0; i<content.size(); i++)
	{
		cout << endl << "i: " << i;
		cout << endl << content[i] << content[i+1] << content[i+2] << content[i+3];
		if(content[i] != path)
		{
			// path
			ofile << content[i];
			bytes_written++;
			i++;
			// " "
			ofile << content[i];
			bytes_written++;
			i++;
			// mtime
			ofile << content[i];
			bytes_written++;
			i++;
			// "\n"
			if(i<content.size()){
				ofile << content[i];
				bytes_written++;
			}
		}
		else
			i+=3;
	}
	ofile.close();

	if(bytes_written==0)
		remove(path_txt.c_str());
}
