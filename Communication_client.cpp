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
    //this->header = (packet*)malloc(header_size);
	//this->header_address = (size_t)header;
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
    std::cout << "\n\npayload: " << pkt._payload << std::endl;

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
    std::cout << "bytes sent: " << bytes_sent << std::endl;

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
    std::cout << "bytes sent: " << bytes_sent << std::endl;


    if(receive_int() < 0) {
        std::cout << "\nConnection refused\nToo many connections\n";
        return connect;
    } else {
        connected = true;
    }
	return connected;
}


void Communication_client::send_command(int command) {
	int n;
	//printf("\nentrei na send_command\n");

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
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    //std::cout << "bytes sent: " << bytes_sent << std::endl;
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
    //std::cout << "bytes sent: " << bytes_sent << std::endl;
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
    std::cout << "\n\n(send_filename) filename: " << pkt._payload << std::endl;

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
    //std::cout << "bytes sent: " << bytes_sent << std::endl;

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
    //std::cout << "bytes sent: " << bytes_sent << std::endl;
}

void Communication_client::send_file(std::string filename, std::string path){
	char* file_buffer = (char*)malloc(payload_size);
	std::string complete_path = path + "/" + filename;
	std::cout << "\n(send_file) PATH COMPLETO:" << complete_path;
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
    //std::cout << "\n\ntotal size: " << total_size;

    int i;
    int total_bytes_sent = 0;
    //std::cout << "\n\nenviando: " << std::endl;
	//printf("%.*s\n", payload_size, buffer);

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

        //std::cout << std::endl << total_bytes_sent << " bytes have been sent";
        //std::cout << std::endl << total_payload_size - (total_bytes_sent - header_size*(i-1)) << " bytes will be sent";

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
        /*std::cout << "\n\nHEADER!\n";
        std::cout << "bytes sent: " << bytes_sent << std::endl;
        std::cout << "type: " << pkt.type;
        std::cout << "\nseqn: " << pkt.seqn;
        std::cout << "\ntotal_size: " << pkt.total_size;
        std::cout << "\npayload_size: " << pkt.length << std::endl;*/

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
        /*std::cout << "PACKET!\n";
        std::cout << "\npayload(char*): ";
        printf("%.*s\n", payload_size, pkt._payload);
        std::cout << "bytes sent: " << bytes_sent << std::endl;*/
    }
	free(file_buffer);
	fclose(fp);
}

void Communication_client::send_mtime(time_t mtime) {
    //std::cout << "\nENTROU NA SEND_MTIME";
    const char* payload = (char*)&mtime;
    int n;

    // Create the packet that will be sent
    packet pkt;
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(time_t);
	pkt._payload = payload;
    //std::cout << "\n\nfilename: " << pkt._payload << std::endl;

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
    //std::cout << "bytes sent: " << bytes_sent << std::endl;

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
    //std::cout << "bytes sent: " << bytes_sent << std::endl;
}

// The type variable defines the type of output:
// 0 -> just return 0 (the pkt will be used by the caller)
// 1 -> command (int)
// 2 -> mtime (time_t)
// Since time_t is a signed integer that can be 32 or 64 bits long (depending on
//the system), we return a long int (64 bits signed integer)
long int Communication_client::receive_payload(struct packet *pkt, int type) {
    //std::cout << "\nENTREI NO RECEIVE_PAYLOAD\n\n";
    receive_header(pkt);
    //std::cout << "Saiu do header\n";
    //std::cout << "pkt->length: " << pkt->length;
    //std::cout << "\nVai entrar no while do receive_payload";
    int bytes_received = 0;
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length - bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");

        bytes_received+=n;
        //std::cout << "\nEstou no while do receive_payload";
		//std::cout << "\nbytes lidos: " << bytes_received << std::endl;
        //std::cout << "\npkt lenght: " << pkt->length;
	}
    //std::cout << "\nbytes lidos: " << bytes_received;
	pkt->_payload = (const char*)buffer;
	switch (type)
	{
		case 1:{
	        int command;
	        memcpy(&command, pkt->_payload, pkt->length);
	        //cout << command;
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

void Communication_client::receive_header(struct packet *_header) {
    buffer = (char*)buffer_address;
    //bzero(buffer, header_size);
    //std::cout << "\n\nENTREI NO RECEIVE_HEADER\n\n";
	int bytes_received=0;
	//cout << "\n\nbytes lidos: "<<bytes_received;
    while(bytes_received < header_size)
    {
        //std::cout << "\nheader size: " << header_size;
        //std::cout << "\nBYTES_LIDOS ANTES DO READ: " << bytes_received;
        //int n = read(sockfd, buffer, header_size);
        int n = read(sockfd, buffer, header_size - bytes_received);
        //std::cout << "\nBYTES_LIDOS DEPOIS DO READ: " << bytes_received;
        //cout << "\nN DEPOIS DO READ: " << n;
        if (n < 0) {
            printf("ERROR reading from socket");
        }

        bytes_received+=n;
		//std::cout << "\nbytes lidos: " << bytes_received;
        //std::cout << "\nheader size: " << header_size;
	}
	if(bytes_received != 0) // No need to copy anything to the header if no bytes were received
	{
	    //cout << "\nKAPOW!\n";
        //cout << "buffer[0]: " << buffer[0] << endl;
        //cout << "buffer[1]: " << buffer[1] << endl;
	    // Bytes from buffer[4] to buffer[7] are the size of _payload
	    memcpy(&_header->type, &buffer[0], 2);
	    memcpy(&_header->seqn, &buffer[2], 2);
	    memcpy(&_header->total_size, &buffer[4], 4);
	    memcpy(&_header->length, &buffer[8], 2);
	    /*std::cout << "\ntype: " << _header->type;
	    std::cout << "\nseqn: " << _header->seqn;
	    std::cout << "\ntotal_size: " << _header->total_size;
	    std::cout << "\npayload_size: " << _header->length << std::endl;*/
    }
    //std::cout << "\nSaindo do receive_header\n";
/*
    std::cout << "\nSai do while do receive_header";
    // Bytes from buffer[4] to buffer[7] are the size of _payload
	struct packet* header;
	header = (packet*)malloc(header_size);
	memcpy(&header->type, &buffer[0], 2);
	memcpy(&header->seqn, &buffer[2], 2);
	memcpy(&header->total_size, &buffer[4], 4);
	memcpy(&header->length, &buffer[8], 2);
	std::cout << "\ntype: " << header->type;
	std::cout << "\nseqn: " << header->seqn;
	std::cout << "\ntotal_size: " << header->total_size;
	std::cout << "\npayload_size: " << header->length << std::endl;
*/
}

int Communication_client::receive_int() {
    packet pkt;
    return receive_payload(&pkt, 1);
}

void Communication_client::receive_file(std::string path) {
    FILE *fp = fopen(path.c_str(), "w");
    if(fp==NULL)
        std::cout << "\nERROR OPENING " << path << std::endl;


    struct packet pkt;
    receive_payload(&pkt, 0);
    uint32_t total_size = pkt.total_size;
    //std::cout << "pkt total size: " << pkt.total_size;
    //std::cout << "\n\nTHE CLIENT WILL RECEIVE " << total_size << " PACKETS!\n";

    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
    //std::cout << "\nComecei a escrever no file pqp";
    if (bytes_written_to_file < pkt.length)
        std::cout << "\nERROR WRITING TO " << path << std::endl;

    //std::cout << bytes_written_to_file << " bytes written to file" << std::endl;

    // Receive all the [total_size] packets
    // It starts at 2 because the first packet has already been received
    int i;
    for(i=2; i<=total_size; i++)
    {
        //std::cout << "\ni: " << i << "/" << total_size;
        // Receive payload
        receive_payload(&pkt, 0);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
        if (bytes_written_to_file < pkt.length)
            std::cout << "\nERROR WRITING TO " << path << std::endl;
        //std::cout << "\n" << bytes_written_to_file << " bytes written to file\n";
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
    //std::cout << "path: " << path;
    error = remove(path.c_str());
    if(error != 0)
        std::cout << "\nError deleting file";
    return error;
}


void Communication_client::upload_command(int command, std::string filename, std::string path, time_t mtime) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

	//send command upload (1)
	send_command(command);

    // resposta do server
    // Receive return int
    if(receive_int() < 0){
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
	//std::cout << "\nENTREI NA DOWNLOAD_COMMAND";
    //std::cout << "\nfilename recebido: " << filename;
    //std::cout << "\npath recebido: " << path;

	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    // send command download (2)
	send_command(command);

    // resposta do server se já fechou ou não
    // Receive return int
    if(receive_int() < 0){
        // se o server fechou não aceita mais nenhum comando do client que não seja o exit
        std::cout << "\nServer closed\n";
        exit(0);
    }

	// send filename
	send_filename(filename);

    if(receive_int() < 0){
        // se entrar aqui é porque o file não existe no server, logo não tem o que ser baixado
        std::cout << "\nFile doesn't exist at server.\n";
        download_file->mtime = -1;
    }
	else{
	    //std::cout << "\nNão devia mas veio mesmo assim ué";
		// receive mtime
	    struct packet pkt;
		time_t mtime = receive_payload(&pkt, 2);

		// receive file
		std::string full_path = path+"/"+filename;
		std::cout << "\n\nDOWNLOAD_COMMAND FULL PATH: " << full_path << "\n\n";
		receive_file(full_path);

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
    if(receive_int() < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

    send_filename(filename);

    //path = path + '/' + filename;
    //std::cout << "\ndelete path: " << path;
    //delete_file(path);

	// Unlock mutex
	pthread_mutex_unlock(&socket_mtx);
}

void Communication_client::list_server_command(int command) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    send_command(command);

    // Receive return int
    if(receive_int() < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

    struct packet pkt;
    receive_payload(&pkt, 0);
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
    if(receive_int() < 0){
        std::cout << "\nServer closed\n";
        exit(0);
    }

    //recebe o numero de arquivos do server
    packet pkt;
	int num_server_files = receive_payload(&pkt, 1);

	//std::cout << "\nFILES ON SERVER (contando os que foram deletados): " << num_server_files << "\n\n";

	std::vector<std::string> file_names;
	std::vector<time_t> mtimes;
    //recebe o filename e o mtime de cada arquivo
    for(int i = 1; i <= num_server_files; i++) {

        // recebe o filename
        receive_payload(&pkt, 0);
        std::string server_filename = pkt._payload;
		server_filename.resize(pkt.length);

		file_names. push_back(server_filename);

        // recebe o mtime
        time_t server_mtime = receive_payload(&pkt, 2);

		mtimes.push_back(server_mtime);

		//std::cout << "\nserver filename: \'" << server_filename << "\'" << std::endl << "server mtime: " << server_mtime << std::endl;
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
        // percorre a watched_files pra achar as inconsistências entra os arquivos no servidor e no client
		for(int j = 0; j < watched_files_size && !found; j++){
            // se acha o arquivo da server na watched_files
			//std::cout << "i: " << i << "\ncomparando com: filename: \'" << (*watched_files)[j].name << "\'" << std::endl << "mtime: " << (*watched_files)[j].mtime << std::endl;
            if((*watched_files)[j].name == server_filename){
				//std::cout << "\nACHOU FILENAME!\n";
				found = 1;
				pos = j;
				break;
			}
		}
		if(found){
			// found = 1
	        if(server_mtime < 0){
	            // se o arquivo foi deletado no servidor, vai ser deletado no cliente
				std::cout << "\n(get_sync_dir) file " << complete_path << "will be deleted\n";
	            delete_file(complete_path);
				(*watched_files)[pos].mtime = -2; // sinaliza que deve ser removido
	            //remove_from_watched_files(server_filename, watched_files);
			  // Se mtime é -1, o arquivo deve ser deletado no servidor
	        } else if((*watched_files)[pos].mtime != -1) {
	            double seconds = difftime(server_mtime, (*watched_files)[pos].mtime);
	            if(seconds > 0) { // Se a versão do server é mais nova
					std::cout << "\n(get_sync_dir) file " << complete_path << " will be downloaded (server version is more recent)\n";
					download_command(2, server_filename, path, &download_file);
	                watched_files->push_back(download_file);
	            }
	            if (seconds < 0) { // Se a versão do client é mais nova
					std::cout << "\n(get_sync_dir) file " << complete_path << " will be uploaded (client version is more recent)\n";
	                upload_command(1, server_filename, path, (*watched_files)[pos].mtime);
	            }
	        } else { // Se mtime do client é -1
				std::cout << "\n(get_sync_dir) file " << complete_path << " will be deleted on the server)\n";
				delete_command(3, server_filename, path);
				(*watched_files)[pos].mtime = -2; // sinaliza que deve ser removido
	            //remove_from_watched_files(server_filename, watched_files);
			}
	    }
		else if (server_mtime != -1){
            // found = 0
			std::cout << "\n(get_sync_dir) file " << complete_path << " will be downloaded (client doesn't have file)\n";
            download_command(2, server_filename, path, &download_file);
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
			std::cout << "\n(get_sync_dir) file " << complete_path << " will be uploaded (server doesn't have file)\n";
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
            //std::cout << "\n\nwatched_file: "<< watched_files[i].name << "\tfilename: " << filename;
            watched_files->erase(watched_files->begin()+i);
    }
}

void Communication_client::exit_command(int command) {
	// Lock mutex to make sure that the other thread won't execute other commands while this command is running
	pthread_mutex_lock(&socket_mtx);

    send_command(7);

    // Receive return int
    if(receive_int() < 0){
        std::cout << "\nServer closed\n";
		// Unlock mutex
		pthread_mutex_unlock(&socket_mtx);
	    close(sockfd);
        exit(0);
    }
}
