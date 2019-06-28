#include "Communication_client.h"

#include <iostream>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

Communication_client::Communication_client() {
	this->payload_size = 502;
	this->header_size = 10;
	this->packet_size = this->header_size + this->payload_size;
	this->buffer = (char*)malloc(header_size);
    this->buffer_address = (size_t)buffer;
}

bool Communication_client::connect_client_server(Client client) {
	int n;
	int port = client.getPort();
	std::string hostname = client.getHostname();
	std::string username = client.getUsername();
	bool connected = false;
    struct sockaddr_in serv_addr; // server_address
    struct hostent *server = gethostbyname(hostname.c_str());

	if (server == NULL) {
		std::cerr << "ERROR, no such host\n";
        return connected;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
		std::cerr << "ERROR opening socket\n";
        return connected;
    }

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);

	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		std::cerr << "ERROR connecting with server\n";
		return connected;
	}

	// SEND USERNAME
	const char* payload = username.c_str();
	packet pkt;
	pkt.type = 2;
 	pkt.seqn = 0;
  	pkt.total_size = 1;
  	pkt.length = strlen(payload);
	pkt._payload = payload;

	buffer = (char*)&pkt;

	// Escreve o header no socket
	int bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size - bytes_sent);
        if (n < 0) {
			std::cerr << "ERROR writing socket\n";
		    return connect;
		}
		bytes_sent += n;
    }

	// Escreve o payload
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) {
			std::cerr << "ERROR writing socket\n";
		    return connect;
		}
		bytes_sent += n;
    }


    if(receive_int(0) < 0) {
        std::cout << "\nConnection refused\nToo many connections\n";
        return connect;
    } else {
        connected = true;
    }
	return connected;
}


bool Communication_client::send_command(int command) {
	int n;

	packet pkt;
	pkt.type = 1;
    pkt.seqn = 0;
    pkt.total_size = 1;
    pkt.length = sizeof(int);
	pkt._payload = (const char*)&command;

	// send header
	// write in the socket
	buffer = (char*)&pkt;
	int bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size - bytes_sent);
        if (n < 0)
		    return false;
		bytes_sent += n;
    }
    //send payload
	// write in the socket
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0)
		    return false;
		bytes_sent += n;
    }
	return true;
}

void Communication_client::send_filename(std::string filename) {
	int n;
	int bytes_sent = 0;
	packet pkt;
	const char* payload = filename.c_str();
	// Create the packet that will be sent
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = filename.size();
	pkt._payload = payload;

	// copy pkt to buffer
	buffer = (char*)&pkt;

	// send header
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size - bytes_sent);
        if (n < 0)
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }

    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length - bytes_sent);
        if (n < 0)
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
}

void Communication_client::send_file(std::string filename, std::string path){
	char* file_buffer = (char*)malloc(payload_size);
	std::string complete_path = path + "/" + filename;
	FILE *fp = fopen(complete_path.c_str(), "r");

	if(fp == NULL)
        std::cout << "Error opening file " << complete_path << std::endl;

    // Get the size of the file
    fseek(fp, 0 , SEEK_END);
    long total_payload_size = ftell(fp);
    // Go back to the beginning
    fseek(fp, 0 , SEEK_SET);

    // The type of the packet being sent is 0 (data)
    uint16_t type = 0;

    // If the data is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)total_payload_size/(float)payload_size;
    int total_size = total_size_f;
    if (total_size_f > total_size)
        total_size ++;

    int i;
    int total_bytes_sent = 0;

    // Send each packet
    // If only one packet will be sent, the program will go through the loop only once
    for(i=1; i<=total_size; i++)
    {
        // Create the packet that will be sent
        packet pkt;
        pkt.type = type;
        pkt.seqn = i;
        pkt.total_size = total_size;

        // If the chunk of the file that will be sent is smaller
        //than the max payload size, send only the size needed
        if(payload_size > total_payload_size - (total_bytes_sent - header_size*(i-1)))
            pkt.length = total_payload_size - (total_bytes_sent - header_size*(i-1));
        else
            pkt.length = payload_size;

        // Read pkt.length bytes from the file
        fread(file_buffer, 1, pkt.length, fp);
        // Save it to pkt._payload
        pkt._payload = file_buffer;

        // Point buffer to pkt
        buffer = (char*)&pkt;

        //------------------------------------------------------------------------
        // SEND HEADER
        //------------------------------------------------------------------------
        // write in the socket
        int bytes_sent = 0;
        while (bytes_sent < header_size)
        {
            int n = write(sockfd, &buffer[bytes_sent], header_size - bytes_sent);
            if (n < 0)
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;

        //------------------------------------------------------------------------
        // SEND PAYLOAD
        //------------------------------------------------------------------------
        // write in the socket
        bytes_sent = 0;
        while (bytes_sent < pkt.length)
        {
            int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length - bytes_sent);
            if (n < 0)
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
    }
	free(file_buffer);
	fclose(fp);
}

void Communication_client::send_mtime(time_t mtime) {
    const char* payload = (char*)&mtime;
    int n;

    // Create the packet that will be sent
    packet pkt;
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(time_t);
	pkt._payload = payload;

	// copy pkt to buffer
	buffer = (char*)&pkt;

	// send header
	// write in the socket
    int bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    n = write(sockfd, &buffer[bytes_sent], header_size - bytes_sent);
        if (n < 0)
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }

    //send payload
	// write in the socket
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0)
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
}

// The type variable defines the type of output:
// 0 -> just return 0 (the pkt will be used by the caller)
// 1 -> command (int)
// 2 -> mtime (time_t)
// Since time_t is a signed integer that can be 32 or 64 bits long (depending on
//the system), we return a long int (64 bits signed integer)
long int Communication_client::receive_payload(struct packet *pkt, int type, int timeout_sec) {
    receive_header(pkt, timeout_sec);
	// Check if timed out
	if(pkt->type == 10)
		return -10;

	bool timedout = false;
	if(timeout_sec != 0){
		// Set up the timeout
		fd_set input;
		FD_ZERO(&input);
		FD_SET(sockfd, &input);
		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		int n = select(sockfd + 1, &input, NULL, NULL, &timeout);
	}

    int bytes_received = 0;
    while(bytes_received < pkt->length && !timedout)
    {
        // read from the socket
        int n = read(sockfd, &buffer[bytes_received], pkt->length - bytes_received);
        if (n<=0 && timeout_sec>0) {
			timedout = true;
        }
		else if(n>0)
        	bytes_received+=n;
	}
	pkt->_payload = (const char*)buffer;
	if(timedout){
		std::cout << std::endl << "TIMEOUT NO RECEIVE_PAYLOAD!";
		pkt->type = 10;
	}
	switch (type)
	{
		case 1:{
	        int command;
	        memcpy(&command, pkt->_payload, pkt->length);
	        return command;
		}
		case 2:{
			time_t mtime= *(time_t*)pkt->_payload;
			return mtime;
		}
		default:
			return 0;
	}
}

void Communication_client::receive_header(struct packet *_header, int timeout_sec) {
	bool timedout = false;
	if(timeout_sec != 0){
		// Set up the timeout
		fd_set input;
		FD_ZERO(&input);
		FD_SET(sockfd, &input);
		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		int n = select(sockfd + 1, &input, NULL, NULL, &timeout);
	}

    buffer = (char*)buffer_address;
	int bytes_received=0;
    while(bytes_received < header_size && !timedout)
    {
        int n = read(sockfd, &buffer[bytes_received], header_size - bytes_received);
        if (n<=0 && timeout_sec>0) {
			timedout = true;
        }
		else if(n>0)
        	bytes_received+=n;
	}
	if(timedout){
		std::cout << std::endl << "TIMEOUT NO RECEIVE_HEADER!";
		_header->type = 10;
	}
	if(bytes_received != 0) // No need to copy anything to the header if no bytes were received
	{
	    memcpy(&_header->type, &buffer[0], 2);
	    memcpy(&_header->seqn, &buffer[2], 2);
	    memcpy(&_header->total_size, &buffer[4], 4);
	    memcpy(&_header->length, &buffer[8], 2);
    }
}

int Communication_client::receive_int(int timeout_sec) {
    packet pkt;
    int return_value = receive_payload(&pkt, 1, timeout_sec);
	if(pkt.type == 10)
		return -10;
	return return_value;
}

void Communication_client::receive_file(std::string path) {
    FILE *fp = fopen(path.c_str(), "w");
    if(fp==NULL)
        std::cout << "\nERROR OPENING " << path << std::endl;


    struct packet pkt;
    receive_payload(&pkt, 0, 0);
    uint32_t total_size = pkt.total_size;

    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
    if (bytes_written_to_file < pkt.length)
        std::cout << "\nERROR WRITING TO " << path << std::endl;

    // Receive all the [total_size] packets
    // It starts at 2 because the first packet has already been received
    int i;
    for(i=2; i<=total_size; i++)
    {
        // Receive payload
        receive_payload(&pkt, 0, 0);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
        if (bytes_written_to_file < pkt.length)
            std::cout << "\nERROR WRITING TO " << path << std::endl;
    }
    fclose(fp);
}

long Communication_client::get_file_size(FILE *fp) {
    // Get file size
    fseek(fp, 0 , SEEK_END);
    long size = ftell(fp);
    // Go back to the beginning
    fseek(fp, 0 , SEEK_SET);

    fclose(fp);
    return size;
}

int Communication_client::delete_file(std::string path) {
    int error = 0;
    error = remove(path.c_str());
    if(error != 0)
        std::cout << "\nError deleting file";
    return error;
}

std::vector<std::string> Communication_client::receive_backups_ip_socket() {
	// Receive backups IPs and sockets
    int num_backups = receive_int(10);
    std::cout << "\nSIZE_BACKUPS: " << num_backups << "\n";

    for(int i=0; i < num_backups; i++) {
        // First receive IP
		packet pkt;
        receive_payload(&pkt, 0, 0);
		std::string ip = pkt._payload;
		std::cout << "BACKUP IP: " << ip << "\n";

		backup_ips.push_back(ip);
    }

	return backup_ips;
}

bool Communication_client::check_server_command(int command){
	bool server_alive = true;

	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

	//send command upload (1)
	server_alive = send_command(command);

	if(!server_alive){
		std::cout << std::endl << "SERVER DIED!!!";
		// Unlock mutex
		pthread_mutex_unlock(&socket_mtx);
		return server_alive;
	}

    // resposta do server
    // Receive return int
	int int_received = receive_int(10);

	if(int_received == -10){
		std::cout << std::endl << "SERVER DEAD!";
		server_alive = false;
	}
	else if(int_received > 0)
		std::cout << std::endl << "SERVER ALIVE!";
    else if(int_received < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);

	return server_alive;
}

void Communication_client::upload_command(int command, std::string filename, std::string path, time_t mtime) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

	//send command upload (1)
	send_command(command);

    // resposta do server
    // Receive return int
    if(receive_int(0) < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

	// send filename
	send_filename(filename);

    // send mtime
    send_mtime(mtime);

	//send file
	send_file(filename, path);

	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);
}

void Communication_client::download_command(int command, std::string filename, std::string path, Client::file *download_file) {

	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    // send command download (2)
	send_command(command);

    // resposta do server se já fechou ou não
    // Receive return int
    if(receive_int(0) < 0){
        // se o server fechou não aceita mais nenhum comando do client que não seja o exit
        std::cout << "\nServer closed\n";
        exit(0);
    }

	// send filename
	send_filename(filename);

    if(receive_int(0) < 0){
        // se entrar aqui é porque o file não existe no server, logo não tem o que ser baixado
        std::cout << "\nFile doesn't exist at server.\n";
        download_file->mtime = -1;
    }
	else{
		// receive mtime
	    struct packet pkt;
		time_t mtime = receive_payload(&pkt, 2, 0);

		// receive file
		receive_file(path);

		download_file->name = filename;
		download_file->mtime = mtime;

		struct stat fileattrib;
		if(stat(path.c_str(), &fileattrib) < 0)
			std::cout << "\nstat error\n";
		download_file->local_mtime = fileattrib.st_mtime;
	}
	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);
}

void Communication_client::delete_command(int command, std::string filename, std::string path) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    send_command(command);

    // resposta do server
    // Receive return int
    if(receive_int(0) < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

    send_filename(filename);

	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);
}

void Communication_client::list_server_command(int command) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    send_command(command);

    // Receive return int
    if(receive_int(0) < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

    struct packet pkt;
    receive_payload(&pkt, 0, 0);
	std::string ls = pkt._payload;
	ls.resize(pkt.length);

	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);
    std::cout << "\n\nlist_server: " << ls << std::endl << std::endl;
}


void Communication_client::get_sync_dir(int command, std::vector<Client::file> *watched_files, std::string path) {
	Client::file download_file;

	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    send_command(command);

    // recebe se o server fechou
    if(receive_int(0) < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

    //recebe o numero de arquivos do server
    packet pkt;
	int num_server_files = receive_payload(&pkt, 1, 0);

	std::vector<std::string> file_names;
	std::vector<time_t> mtimes;
    //recebe o filename e o mtime de cada arquivo
    for(int i = 1; i <= num_server_files; i++) {

        // recebe o filename
        receive_payload(&pkt, 0, 0);
        std::string server_filename = pkt._payload;
		server_filename.resize(pkt.length);

		file_names. push_back(server_filename);

        // recebe o mtime
        time_t server_mtime = receive_payload(&pkt, 2, 0);

		mtimes.push_back(server_mtime);
	}
	// Unlock the mutex (The interaction with the server for this command ends here. Now it will execute other commands).
	pthread_mutex_unlock(&socket_mtx);

	// Verifica cada nome e mtime recebido contra os arquivos locais
	for(int i = 0; i < num_server_files; i++) {
		std::string server_filename = file_names[i];
		time_t server_mtime = mtimes[i];
		std::string complete_path = path + '/' + server_filename;

        int watched_files_size = watched_files->size();
	    int found = 0;
		int pos = 0;
        // percorre a watched_files pra achar as inconsistências entre os arquivos no servidor e no client
		for(int j = 0; j < watched_files_size && !found; j++){
            // se acha o arquivo da server na watched_files
            if((*watched_files)[j].name == server_filename){
				found = 1;
				pos = j;
				break;
			}
		}
		if(found){
			// found = 1
	        if(server_mtime < 0){
	            // se o arquivo foi deletado no servidor, vai ser deletado no cliente
				//std::cout << "\n(get_sync_dir) file " << complete_path << "will be deleted\n";
	            delete_file(complete_path);
				(*watched_files)[pos].mtime = -2; // sinaliza que deve ser removido
			  // Se mtime é -1, o arquivo deve ser deletado no servidor
	        } else if((*watched_files)[pos].mtime != -1) {
	            double seconds = difftime(server_mtime, (*watched_files)[pos].mtime);
	            if(seconds > 0) { // Se a versão do server é mais nova
					//std::cout << "\n(get_sync_dir) file " << complete_path << " will be downloaded (server version is more recent)\n";
					download_command(2, server_filename, complete_path, &download_file);
					// Update mtime
					(*watched_files)[pos].mtime = download_file.mtime;
	                (*watched_files)[pos].local_mtime = download_file.local_mtime;
	            }
	            if (seconds < 0) { // Se a versão do client é mais nova
					//std::cout << "\n(get_sync_dir) file " << complete_path << " will be uploaded (client version is more recent)\n";
	                upload_command(1, server_filename, path, (*watched_files)[pos].mtime);
	            }
	        } else { // Se mtime do client é -1
				//std::cout << "\n(get_sync_dir) file " << complete_path << " will be deleted on the server)\n";
				delete_command(3, server_filename, path);
				(*watched_files)[pos].mtime = -2; // sinaliza que deve ser removido
			}
	    }
		else if (server_mtime != -1){
            // found = 0
			//std::cout << "\n(get_sync_dir) file " << complete_path << " will be downloaded (client doesn't have file)\n";
            download_command(2, server_filename, complete_path, &download_file);
            watched_files->push_back(download_file);
        }
    }

	// Percorre todos os watched_files
	for(int i=0; i<watched_files->size(); i++){
		// Se o arquivo é novo
		if((*watched_files)[i].mtime==0){
			(*watched_files)[i].mtime = (*watched_files)[i].local_mtime;
			std::string complete_path = path + '/' + (*watched_files)[i].name;
			// Envia
			//std::cout << "\n(get_sync_dir) file " << complete_path << " will be uploaded (server doesn't have file)\n";
			upload_command(1, (*watched_files)[i].name, path, (*watched_files)[i].mtime);
		}
		// Se o arquivo deve ser removido
		else if((*watched_files)[i].mtime == -2){
			// Remove
			watched_files->erase(watched_files->begin()+i);
			i--;
		}
	}
}

void Communication_client::remove_from_watched_files(std::string filename, std::vector<Client::file> *watched_files) {
    for(int i=0; i < watched_files->size(); i++)
    {
        if(filename == (*watched_files)[i].name)
            watched_files->erase(watched_files->begin()+i);
    }
}

void Communication_client::exit_command(int command) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    send_command(7);

    // Receive return int
    if(receive_int(0) < 0){
        std::cout << "\nServer closed\n";
    }
	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);
}

void Communication_client::close_socket(){
	close(sockfd);
}
