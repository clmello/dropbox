#include "Communication_server.h"

using namespace std;

// This global variable tells all the threads in the server that the server will close
extern bool closing_server;

void Communication_server::Init(int port, int header_size, int max_payload)
{
	this->port = port;
	this->header_size = header_size;
	this->max_payload = max_payload;
	this->packet_size = this->header_size + this->max_payload;
	//cout << "\nchamando aceita_conexoes\n";
	//accept_connections();
}

void Communication_server::receive_header(int sockfd, struct packet *header)
{
	char *buffer = (char*)malloc(header_size);
    //cout << "\n\n" << sockfd << ": ENTREI NO RECEIVE_HEADER\n\n";
	int bytes_received=0;
	//cout << "\n\nbytes lidos: "<<bytes_received;
    while(bytes_received < header_size)
    {
        //cout << "\n\nsockfd = " << sockfd << "\n\n";
        /* read from the socket */
        //cout << "\nBYTES_LIDOS ANTES DO READ: " << bytes_received;
        int n = read(sockfd, &buffer[bytes_received], header_size-bytes_received);
        //cout << "\nBYTES_LIDOS DEPOIS DO READ: " << bytes_received;
        //cout << "\nN DEPOIS DO READ: " << n;
        if (n < 0)
            printf("ERROR reading from socket");

        bytes_received+=n;
		//cout << "\n" << sockfd << ": bytes lidos: "<<bytes_received;
	}
	if(bytes_received != 0) // No need to copy anything to the header if no bytes were received
	{
	    //cout << "\nKAPOW!\n";
        //cout << "buffer[0]: " << buffer[0] << endl;
        //cout << "buffer[1]: " << buffer[1] << endl;
	    // Bytes from buffer[4] to buffer[7] are the size of _payload
	    memcpy(&header->type, &buffer[0], 2);
	    memcpy(&header->seqn, &buffer[2], 2);
	    memcpy(&header->total_size, &buffer[4], 4);
	    memcpy(&header->length, &buffer[8], 2);
	    //cout << "\n" << sockfd << ": type: " << header->type;
	    //cout << "\n" << sockfd << ": seqn: " << header->seqn;
	    //cout << "\n" << sockfd << ": total_size: " << header->total_size;
	    //cout << "\n" << sockfd << ": payload_size: " << header->length << endl;
    }
    free(buffer);
}

// The type variable defines the type of payload being received:
// 0 -> not defined
// 1 -> command
// 2 -> mtime
long int Communication_server::receive_payload(int sockfd, struct packet *pkt, int type)
{
	char *buffer = (char*)malloc(max_payload);
    //cout << "\n\n" << sockfd << ": ENTREI NO RECEIVE_PAYLOAD\n\n";
    receive_header(sockfd, pkt);
	int bytes_received=0;
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, &buffer[bytes_received], pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");

        bytes_received+=n;
		//cout << "\n" << sockfd << ": bytes lidos: "<<bytes_received<<endl;
	}
	//cout << "\n" << sockfd << ": bytes lidos: "<<bytes_received;
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
	/*
	if(pkt->type != 1){ // If the packet is not a command
		time_t mtime2 = *(time_t*)pkt->_payload;
		std::cout << "\n\nRECEBENDO MTIME: " << mtime2 << std::endl << std::endl;
	    //cout << "\n" << sockfd << ": payload(char*): ";
	    //printf("%.*s\n", max_payload, pkt->_payload);
    }
    else{ // If the packet is a command
	    //cout << "\n" << sockfd << ": payload(int): ";
        int command;
        memcpy(&command, pkt->_payload, pkt->length);
        //cout << command;
        return command;
    }
    //cout << endl << endl;
	free(buffer);
	return 0;*/
}

void *Communication_server::receive_commands(int sockfd, string username, int *thread_finished, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex, vector<int> *backup_sockets, vector<pthread_mutex_t> *backup_mutexes, pthread_mutex_t *r_w_backups_mutex, vector<int> *r_w_backups)
{
    bool close_thread = false;

	files_from_disk(username, user_files, user_files_mutex);

	//--------------------------------------------------------------
	// Send username to all backups
	//--------------------------------------------------------------
	// We need to guarantee that no new backups will connect while
	//the next for executes
	/*lock_rw_mutex(r_w_backups_mutex, r_w_backups);

	// Send username
	for(int i=0; i<backup_mutexes->size(); i++)
	{
		// Send command
		send_int((*backup_sockets)[i], 3);
		// Send username
		send_string((*backup_sockets)[i], username);
	}

	// Stop reading
	unlock_rw_mutex(r_w_backups_mutex, r_w_backups);*/
	//--------------------------------------------------------------


    while(!close_thread) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        // Wait for a command
        cout << endl << sockfd << ": waiting for command";
        struct packet pkt;
        int command = receive_payload(sockfd, &pkt, 1);


		// Here, after every command the server sends a code (int) to the user:
		//  1: everything OK. The command will be executed;
		// -1: server is closing. The command will not be executed;
        if(closing_server){
            send_int(sockfd, -1);
            command = 7;
        }
		/*else if(file_not_found){
			file_not_found = false;
            send_int(sockfd, -2);
        }*/
        else{
            send_int(sockfd, 1);
        }

        switch(command)
        {
            case 1: // Upload to server
            {
                cout << endl << sockfd << ": command 1 received";

                string path = getenv("HOME");
                // Receive the file name
                receive_payload(sockfd, &pkt, 0);
                string filename = pkt._payload;
				filename.resize(pkt.length);
                path = path + "/server_sync_dir_" + username + "/" + filename;
                //cout << "String path: " << path << endl;

                // Receive mtime
                time_t mtime = receive_payload(sockfd, &pkt, 2);
                // Save mtime to file
				cout << endl << "file path: " << path;
                mtime_to_file(path, mtime, user_files_mutex, username);

				// This function locks the file mutex as a writer
				start_writing_file(path, user_files, user_files_mutex, mtime);

                // Receive the file
                receive_file(sockfd, path);

				// This function unlocks the mutex as a writer
				done_writing_file(path, user_files, user_files_mutex);

				//--------------------------------------------------------------
				// Backups
				//--------------------------------------------------------------
				// Send file to backups
				// We need to guarantee that no new backups will connect while
				//the next for executes
				lock_rw_mutex(r_w_backups_mutex, r_w_backups);

				// Send file to all backups
				for(int i=0; i<backup_mutexes->size(); i++)
				{
					// Send command
					send_int((*backup_sockets)[i], 1);
					// Send username
					send_string((*backup_sockets)[i], username);
					// Send filename
					send_string((*backup_sockets)[i], filename);
					// Send file
					send_file((*backup_sockets)[i], path);
				}

				// Stop reading
				unlock_rw_mutex(r_w_backups_mutex, r_w_backups);
				//--------------------------------------------------------------

                break;
            }
            case 2: // Download from server
            {
                cout << endl << sockfd << ": command 2 received";

                string path = getenv("HOME");
                // Receive the file name
                receive_payload(sockfd, &pkt, 0);
                string filename = pkt._payload;
				filename.resize(pkt.length);
                path = path + "/server_sync_dir_" + username + "/" + filename;
                //cout << "\npath: " << path;

				//TODO: Testa se arquivo existe. Se não existe, retorna erro e não faz mais nada daqui do download

				//TODO: Pede para ler arquivo (mutex)
				// This function locks the file mutex as a reader
				int return_value = start_reading_file(path, user_files, user_files_mutex);

				// If the file doesn't exist, don't try to read it and tell the client that it doesn't exist
				// Send 1 if OK, send -1 if file doesn't exist
				if(return_value == -1){
					//file_not_found = true;
					send_int(sockfd, -1);
					break;
				}
				else
					send_int(sockfd, 1);

                //Send mtime
                time_t mtime = get_mtime(filename, username, user_files, user_files_mutex);
				//cout << endl << endl << "SENDING MTIME: " << mtime << endl << endl;
                send_mtime(sockfd, mtime);

                // Send file
                send_file(sockfd, path);

				//TODO: libera leitura do arquivo
				// This functions unlocks the mutex as a reader
				done_reading_file(path, user_files, user_files_mutex);

                break;
            }
            case 3: // Delete file
            {
                cout << endl << sockfd << ": command 3 received";

                string path = getenv("HOME");
                receive_payload(sockfd, &pkt, 0);
                string filename = pkt._payload;
				filename.resize(pkt.length);
                path = path + "/server_sync_dir_" + username + "/" + filename;
                cout << "String path: " << path;

				//TODO: pede para escrever no arquivo
				// This function locks the file mutex as a writer
				start_writing_file(path, user_files, user_files_mutex, 0);

				// If the file doesn't exist, there is no need to delete it
				if(!file_exists(path, user_files, user_files_mutex)){
					// This function unlocks the mutex as a writer
					done_writing_file(path, user_files, user_files_mutex);
					break;
				}

                delete_file(path);
				// Remove file from the user_files vector
				remove_file(path, user_files, user_files_mutex);
				// This function unlocks the mutex as a writer
				done_writing_file(path, user_files, user_files_mutex);

				//--------------------------------------------------------------
				// Backups
				//--------------------------------------------------------------
				// Delete file on backups
				// We need to guarantee that no new backups will connect while
				//the next for executes
				lock_rw_mutex(r_w_backups_mutex, r_w_backups);

				// Send command to all backups
				for(int i=0; i<backup_mutexes->size(); i++)
				{
					// Send command
					send_int((*backup_sockets)[i], 2);
					// Send username
					send_string((*backup_sockets)[i], username);
					// Send filename
					send_string((*backup_sockets)[i], filename);
				}

				// Stop reading
				unlock_rw_mutex(r_w_backups_mutex, r_w_backups);
				//--------------------------------------------------------------

                break;
            }
            case 4: // List server
            {
                cout << endl << sockfd << ": command 4 received";

				/*// The POSIX readdir page(http://pubs.opengroup.org/onlinepubs/007908799/xsh/readdir.html) establishes:
				//"If a file is removed from or added to the directory after the most recent call to opendir() or
				//rewinddir(), whether a subsequent call to readdir() returns an entry for that file is unspecified."
				// Therefore, it is possible that the user will receive the name of a file that has been deleted while
				//listing. A subsequent list_server command would not list the file, so we have decided tha it would
				//not be a problem.
                string path = getenv("HOME");
                path = path + "/server_sync_dir_" + username;
                DIR *fileDir;
                struct dirent *lsdir;
                fileDir = opendir(path.c_str());
                string return_str = "";
                while ((lsdir = readdir(fileDir)) != NULL)
                {
                    if(lsdir->d_name[0] != '.')
                        return_str = return_str + lsdir->d_name + " ";
                }
                if(return_str != "")
                    return_str.pop_back();
                else
                    return_str = "The folder is empty";
                closedir(fileDir);
                send_string(sockfd, return_str);*/
				string ls = get_files_and_mtime(user_files, user_files_mutex);
				send_string(sockfd, ls);

                break;
            }
            case 6: // Get sync_dir
            {
                cout << endl << sockfd << ": command 6 received";

                // Send number of files to the client
				int number_of_files = (*user_files).size();
				send_int(sockfd, number_of_files);
                //cout << "\nnumber of files: " << number_of_files << endl;

                // Send the name of each file and its mtime
				// Since any thread of the same user could be editing the user_files vector,
				//we need mutual exclusion
				pthread_mutex_lock(user_files_mutex);

				// Go through all the files and send the name of each file and its mtime
				for(int i=0; i<user_files->size(); i++)
				{
					// Send filename
					string path = (*user_files)[i].get_path();
					send_string(sockfd, path.substr(path.find_last_of("\\/")+1, path.length()));

					// Send mtime
					time_t mtime = (*user_files)[i].get_mtime();
					send_mtime(sockfd, mtime);
				}

				// Unlock the mutex for editing the user_files vector
				pthread_mutex_unlock(user_files_mutex);

				//TODO: receive files from the client, if they are more recent
				// Na real não precisa. Só manda os mtimes pro cliente. Ele decide
				//se quer fazer download/upload de algum arquivo.

                break;
            }
            case 7: // Exit
            {
                // If the server chose to exit, the client did not send command 7
                if(!closing_server)
                    cout << endl << sockfd << ": command 7 received";

                close_thread = true;

                //Tell the main thread that this thread has finished
                *thread_finished = 1;

                // The moment the thread exits this function, it will be terminated
                break;
            }
            case 10: // client sends this command to server to check is server is still aliver
            {
                cout << endl << sockfd << ": command 10 received";
                break;
            }
            default:
            {
            }
        }
	}
}

void *Communication_server::receive_commands_helper(void* void_args)
{
    th_args* args = (th_args*)void_args;
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd, *args->username, args->thread_finished, args->user_files, args->user_files_mutex, args->backup_sockets, args->backup_mutexes, args->r_w_backups_mutex, args->r_w_backups);
    return 0;
}

void Communication_server::send_string(int sockfd, string str)
{
	//cout << "\nsending string: " << str;
    char* buffer = (char*)malloc(packet_size);
    // If the string is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)str.size()/(float)max_payload;
    int total_size = total_size_f;
    if (total_size_f > total_size)
        total_size ++;
    //cout << "\n\ntotal size: " << total_size;

    int i;
    int total_bytes_sent = 0;
    //char *str_buff = (char*)malloc(str.size());
    //cout << "\n\nenviando: " << endl << str;

    // Send each packet
    // If only one packet will be sent, the program will go through the loop only once
    for(i=1; i<=total_size; i++)
    {
        // Create the packet that will be sent
        struct packet pkt;
        pkt.type = 0;
        pkt.seqn = i;
        pkt.total_size = total_size;

        // If the chunk of the file that will be sent is smaller
        //than the max payload size, send only the size needed
        if(max_payload > str.size() - (total_bytes_sent - header_size*(i-1)))
            pkt.length = str.size() - (total_bytes_sent - header_size*(i-1));
        else
            pkt.length = max_payload;

        //cout << endl << total_bytes_sent << " bytes have been sent";
        //cout << endl << str.size() - (total_bytes_sent - header_size*(i-1)) << " bytes will be sent";

        // Read pkt.length characters from the string
        //and save it to pkt._payload
        char str_buff[pkt.length];
        strcpy(str_buff, str.substr((i-1)*max_payload, pkt.length).c_str());
        pkt._payload = (const char*)&str_buff;
        //cout << "\npkt.payload: " << pkt._payload;

        // Point buffer to pkt
        buffer = (char*)&pkt;

        //------------------------------------------------------------------------
        // SEND HEADER
        //------------------------------------------------------------------------
        // write in the socket
        int bytes_sent = 0;
        while (bytes_sent < header_size)
        {
            int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
            if (n > 0)
	        	bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        //cout << "\n\nHEADER!\n";
        //cout << "bytes sent: " << bytes_sent << endl;
        //cout << "type: " << pkt.type;
        //cout << "\nseqn: " << pkt.seqn;
        //cout << "\ntotal_size: " << pkt.total_size;
        //cout << "\npayload_size: " << pkt.length << endl;

        //------------------------------------------------------------------------
        // SEND PAYLOAD
        //------------------------------------------------------------------------
        // write in the socket
        bytes_sent = 0;
        while (bytes_sent < pkt.length)
        {
            int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
            if (n > 0)
		       bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        //cout << "PACKET!\n";
        //cout << "\npayload(char*): ";
        //printf("%.*s\n", max_payload, pkt._payload);
        //cout << "bytes sent: " << bytes_sent << endl;
        //------------------------------------------------------------------------
    }
    //free(buffer);
}

void Communication_server::send_int(int sockfd, int number)
{
    char* buffer;// = (char*)malloc(packet_size);
    // Create the packet that will be sent
    struct packet pkt;
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(int);
    pkt._payload = (const char*)&number;

    // Point buffer to pkt
    buffer = (char*)&pkt;

    //------------------------------------------------------------------------
    // SEND HEADER
    //------------------------------------------------------------------------
    // write in the socket
    int bytes_sent = 0;
    while (bytes_sent < header_size)
    {
        int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
        if (n < 0)
            printf("ERROR writing to socket\n");
        bytes_sent += n;
    }

    //------------------------------------------------------------------------
    // SEND PAYLOAD
    //------------------------------------------------------------------------
    // write in the socket
    bytes_sent = 0;
    while (bytes_sent < pkt.length)
    {
        int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0)
            printf("ERROR writing to socket\n");
        bytes_sent += n;
    }
}

void Communication_server::send_mtime(int sockfd, time_t mtime) {
    const char* payload = (char*)&mtime;
    int n;

    // Create the packet that will be sent
    packet pkt;
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = sizeof(time_t);
	pkt._payload = payload;

	time_t mtime2 = *(time_t*)pkt._payload;
	//std::cout << "\n\nENVIANDO MTIME: " << mtime2 << std::endl << std::endl;

	// copy pkt to buffer
	char* buffer = (char*)&pkt;

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
	time_t mtime3 = *(time_t*)pkt._payload;
	//std::cout << "\n\nENVIANDO MTIME: " << mtime3 << std::endl << std::endl;
    //std::cout << "bytes sent: " << bytes_sent << std::endl;
}

void Communication_server::send_file(int sockfd, string path)
{
    char* buffer;// = (char*)malloc(packet_size);
    //------------------------------------------------------------------------
    // SEND FILE
    //------------------------------------------------------------------------
    FILE *fp = fopen(path.c_str(), "r");
    if(fp == NULL)
        cout << "Error opening file " << path << endl;

    // Get the size of the file
    fseek(fp, 0 , SEEK_END);
    long total_payload_size = ftell(fp);
    // Go back to the beggining
    fseek(fp, 0 , SEEK_SET);

    // The type of the packet being sent is 0 (data)
    uint16_t type = 0;

    // If the data is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)total_payload_size/(float)max_payload;
    int total_size = total_size_f;
    if (total_size_f > total_size)
        total_size ++;
    //cout << "\n\ntotal size: " << total_size;

    int i;
    int total_bytes_sent = 0;
    char *file_buffer = (char*)malloc(max_payload);
    //cout << "\n\nenviando: " << endl;
	//printf("%.*s\n", max_payload, buffer);

    // Send each packet
    // If only one packet will be sent, the program will go through the loop only once
    for(i=1; i<=total_size; i++)
    {
        // Create the packet that will be sent
        struct packet pkt;
        pkt.type = type;
        pkt.seqn = i;
        pkt.total_size = total_size;

        // If the chunk of the file that will be sent is smaller
        //than the max payload size, send only the size needed
        if(max_payload > total_payload_size - (total_bytes_sent - header_size*(i-1)))
            pkt.length = total_payload_size - (total_bytes_sent - header_size*(i-1));
        else
            pkt.length = max_payload;

        //cout << endl << total_bytes_sent << " bytes have been sent";
        //cout << endl << total_payload_size - (total_bytes_sent - header_size*(i-1)) << " bytes will be sent";

        // Read pkt.length bytes from the file
        size_t bytes_read = fread(file_buffer, 1, pkt.length, fp);
        if(bytes_read != pkt.length)
            cout << "\nError reading from file \"" << path << "\"";

        //cout << "\nbytes read: " << bytes_read;
        //cout << "\nConteudo lido: ";
	    //printf("%.*s\n", max_payload, file_buffer);
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
            int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
            if (n < 0)
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        //cout << "\n\nHEADER!\n";
        //cout << "bytes sent: " << bytes_sent << endl;
        //cout << "type: " << pkt.type;
        //cout << "\nseqn: " << pkt.seqn;
        //cout << "\ntotal_size: " << pkt.total_size;
        //cout << "\npayload_size: " << pkt.length << endl;

        //------------------------------------------------------------------------
        // SEND PAYLOAD
        //------------------------------------------------------------------------
        // write in the socket
        bytes_sent = 0;
        while (bytes_sent < pkt.length)
        {
            int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
            if (n < 0)
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        /*cout << "PACKET!\n";
        cout << "\npayload(char*): ";
        printf("%.*s\n", max_payload, pkt._payload);
        cout << "bytes sent: " << bytes_sent << endl;*/
        //------------------------------------------------------------------------
    }
    //free(file_buffer);
    fclose(fp);
}


void Communication_server::receive_file(int sockfd, string path)
{
    FILE *fp = fopen(path.c_str(), "w");
    if(fp==NULL)
        cout << "\nERROR OPENING " << path << endl;

    // Get the number of packets to be received
    // To do that, we must receive the first packet
    struct packet pkt;
    receive_payload(sockfd, &pkt, 0);
    uint32_t total_size = pkt.total_size;
    //cout << "\n\nTHE SERVER WILL RECEIVE " << total_size << " PACKETS!\n";
    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
    if (bytes_written_to_file < pkt.length)
        cout << "\nERROR WRITING TO " << path << endl;

    //cout << bytes_written_to_file << " bytes written to file" << endl;

    // Receive all the [total_size] packets
    // It starts at 2 because the first packet has already been received
    int i;
    for(i=2; i<=total_size; i++)
    {
        // Receive payload
        receive_payload(sockfd, &pkt, 0);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
        if (bytes_written_to_file < pkt.length)
            cout << "\nERROR WRITING TO " << path << endl;
        //cout << "\n" << bytes_written_to_file << " bytes written to file\n";
    }
    fclose(fp);
}

int Communication_server::create_folder(string path)
{
    DIR* dir = opendir(path.c_str());
    //cout << "\npath: " << path << endl;
    if(!dir)
    {
        string command = "mkdir -p " + path;
        int error = system(command.c_str());
        if(error < 0)
            return -1;
    }
    closedir(dir);
    return 0;
}

int Communication_server::delete_folder(string path)
{
    DIR* dir = opendir(path.c_str());
    if(dir)
    {
        string command = "rm -r " + path;
        //cout << "\ncommand: " << command << endl;
        int error = system(command.c_str());
        if(error < 0)
            return -1;
    }
    closedir(dir);
    return 0;
}

int Communication_server::delete_file(string path)
{
    int error = 0;
    error = remove(path.c_str());
    if(error != 0)
        cout << "\nError deleting file";
    return error;
}

long Communication_server::get_file_size(FILE *fp)
{
    // Get file size
    fseek(fp, 0 , SEEK_END);
    long size = ftell(fp);
    // Go back to the beginning
    fseek(fp, 0 , SEEK_SET);

    fclose(fp);
    return size;
}
/*
bool Communication_server::file_is_watched(string filename)
{
    bool file_found = false;
    for(int i=0; i < watched_files.size(); i++)
    {
        if(filename == watched_files[i].name)
            file_found = true;
    }
    return file_found;
}
void Communication_server::update_watched_file(string filename, time_t mtime)
{
    if(!file_is_watched(filename)){
        struct file new_file;
        new_file.name = filename;
        new_file.mtime = mtime;
        watched_files.push_back(new_file);
    }
    else{
        for(int i=0; i < watched_files.size(); i++)
        {
            if(filename == watched_files[i].name)
            {
                if(mtime > watched_files[i].mtime)
                    watched_files[i].mtime = mtime;
                break;
            }
        }
    }
}
time_t Communication_server::get_mtime(string filename)
{
    for(int i=0; i < watched_files.size(); i++)
    {
        if(filename == watched_files[i].name)
            return watched_files[i].mtime;
    }
    return -1;
}
void Communication_server::remove_watched_file(string filename)
{
    for(int i=0; i < watched_files.size(); i++)
    {
        if(filename == watched_files[i].name)
            watched_files.erase(watched_files.begin()+i);
    }
}/*
// ACHO QUE NÂO PRECISA DESSA FUNÇÃO!!!
/*
pthread_mutex_t *Communication_server::get_file_mutex(vector<File_server> *user_files, string path, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);
	// Look for the file with this path
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			// Unlock the mutex for editing the user_files vector
			//pthread_mutex_unlock(user_files_mutex);
			//TODO: pra que fazer isso? ^
			// Return the mutex address
			return (*user_files)[i].get_mutex();
		}
	}
	// Unlock the mutex for editing the user_files vector
	pthread_mutex_unlock(user_files_mutex);
	// If there is no file with this path, return NULL
	return NULL;
}*/

bool Communication_server::file_exists(string path, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			// Unlock the mutex for editing the user_files vector
			pthread_mutex_unlock(user_files_mutex);
			return true;
		}
	}

	// Unlock the mutex for editing the user_files vector
	pthread_mutex_unlock(user_files_mutex);

	return false;
}

void Communication_server::update_user_file(string path, time_t mtime, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path)
			(*user_files)[i].set_mtime(mtime);
	}

	// Unlock the mutex for editing the user_files vector
	pthread_mutex_unlock(user_files_mutex);
}

// This functions locks the file mutex and updates the file mtime
// It returns 1 if OK, -1 if file doesn't exist
int Communication_server::start_reading_file(string path, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	bool file_found=false;
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			file_found=true;
			File_server *file_buffer = &(*user_files)[i];
			// Unlock the mutex for editing the user_files vector
			pthread_mutex_unlock(user_files_mutex);

			// This function will lock the file mutex
			file_buffer->start_reading();
			return 1;
		}
	}

	// Unlock the mutex for editing the user_files vector
	if(!file_found){
		pthread_mutex_unlock(user_files_mutex);
		return -1;
	}
}

void Communication_server::done_reading_file(string path, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	bool file_found = false;
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			file_found = true;
			File_server *file_buffer = &(*user_files)[i];
			// Unlock the mutex for editing the user_files vector
			pthread_mutex_unlock(user_files_mutex);

			// This function will unlock the file mutex
			file_buffer->done_reading();
		}
	}

	// Unlock the mutex for editing the user_files vector
	if(!file_found)
		pthread_mutex_unlock(user_files_mutex);
}

void Communication_server::start_writing_file(string path, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex, time_t mtime)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	bool file_found=false;
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			file_found=true;

			// Update mtime
			(*user_files)[i].set_mtime(mtime);

			File_server *file_buffer = &(*user_files)[i];
			// Unlock the mutex for editing the user_files vector
			pthread_mutex_unlock(user_files_mutex);

			// This function will lock the file mutex
			file_buffer->start_writing();
		}
	}

	// If the file is new, create its entry in the user_files vector
	if(!file_found)
	{
		// Create the new File_server object and add the new file to the user_files vector
		(*user_files).emplace_back(path, mtime);

		File_server *file_buffer = &(*user_files).back();
		// Unlock the mutex for editing the user_files vector
		pthread_mutex_unlock(user_files_mutex);

		// This function will lock the file mutex
		file_buffer->start_writing();
	}
}

void Communication_server::done_writing_file(string path, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	bool file_found = false;
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			file_found = true;
			File_server *file_buffer = &(*user_files)[i];
			// Unlock the mutex for editing the user_files vector
			pthread_mutex_unlock(user_files_mutex);

			// This function will unlock the file mutex
			file_buffer->done_writing();
		}
	}

	// Unlock the mutex for editing the user_files vector
	if(!file_found)
		pthread_mutex_unlock(user_files_mutex);
}

void Communication_server::remove_file(string path, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			// Remove the File_server object from the vector
			//(*user_files).erase((*user_files).begin()+i);
			// Set mtime to -1
			(*user_files)[i].set_mtime(-1);
		}
	}

	// Unlock the mutex for editing the user_files vector
	pthread_mutex_unlock(user_files_mutex);
}

string Communication_server::get_files_and_mtime(vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Go through all the files in the vector
	stringstream strstream;
	strstream.str("");
	for(int i=0; i<user_files->size(); i++)
	{
		//cout << endl << (*user_files)[i].get_path() << " " << (*user_files)[i].get_mtime();
		if((*user_files)[i].get_mtime() > 0)
		{
			//cout << "\nENTREI!\n";
			// Add the file name to the return string
			string path = (*user_files)[i].get_path();
			strstream << path.substr(path.find_last_of("\\/")+1, path.length());

			// Add the mtime to the return string
			time_t t = (*user_files)[i].get_mtime();
	        struct tm lt;
	        localtime_r(&t, &lt);
	        char timebuf[80];
	        strftime(timebuf, sizeof(timebuf), "%c", &lt);
			strstream << "(" << timebuf << ") ";
		}
	}
	string return_str = strstream.str();
	if(return_str != "")
		return_str.pop_back();
	else
		return_str = "The folder is empty";

	//cout << "\nreturn str: " << return_str;

	// Unlock the mutex for editing the user_files vector
	pthread_mutex_unlock(user_files_mutex);

	return return_str;
}

time_t Communication_server::get_mtime(string filename, string username, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	string path = getenv("HOME");
	path = path + "/server_sync_dir_" + username + "/" + filename;

	// Since any thread of the same user could be editing the user_files vector,
	//we need mutual exclusion
	pthread_mutex_lock(user_files_mutex);

	// Look for the file with this path
	for(int i=0; i<user_files->size(); i++)
	{
		if((*user_files)[i].get_path() == path){
			// Unlock the mutex for editing the user_files vector
			pthread_mutex_unlock(user_files_mutex);
			// return mtime
			return (*user_files)[i].get_mtime();
		}
	}

	// Unlock the mutex for editing the user_files vector
	pthread_mutex_unlock(user_files_mutex);
	return -1;
}

void Communication_server::lock_rw_mutex(pthread_mutex_t *r_w_backups_mutex, vector<int> *r_w_backups)
{
	while(true)
	{
		pthread_mutex_lock(r_w_backups_mutex);
		// If no one writing, continue
		if((*r_w_backups)[1] == 0)
			break;
		pthread_mutex_unlock(r_w_backups_mutex);
	}
	// Reading = true
	(*r_w_backups)[0]++;
	pthread_mutex_unlock(r_w_backups_mutex);
}

void Communication_server::unlock_rw_mutex(pthread_mutex_t *r_w_backups_mutex, vector<int> *r_w_backups)
{
	pthread_mutex_lock(r_w_backups_mutex);
	// Stop reading
	(*r_w_backups)[0]--;
	pthread_mutex_unlock(r_w_backups_mutex);
}


void *Communication_server::signal_server_alive_helper(void *void_args)
{
    chk_alive_args* args = (chk_alive_args*)void_args;
    ((Communication_server*)args->obj)->signal_server_alive(*args->sockfd);
}
void *Communication_server::signal_server_alive(int sockfd)
{
	while(!closing_server)
	{
		send_string(sockfd, "alive");
		sleep(5);
	}
}

void Communication_server::mtime_to_file(string path, time_t mtime, pthread_mutex_t *user_files_mutex, string username)
{
	// Lock mutex
	pthread_mutex_lock(user_files_mutex);

	// Set txt path
	string path_txt = getenv("HOME");
	path_txt += "/server_sync_dir_" + username + "/" + "mtimes";

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
	//if(exists)
	//	content.pop_back();

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
		}
		ofile.close();
	}
	// Unlock mutex
	pthread_mutex_unlock(user_files_mutex);
}

void Communication_server::files_from_disk(string username, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)
{
	// Lock mutex
	pthread_mutex_lock(user_files_mutex);

	// Create (or open) mtimes file
	string mtimes_file_path = getenv("HOME");
	mtimes_file_path += "/server_sync_dir_" + username + "/" + "mtimes";
    ifstream mtimes_file(mtimes_file_path.c_str());
    bool exists = mtimes_file.good();
    // If the file exists, get information from the mtimes file
    if(exists)
	{
		// The structure of the file is:
		//path_to_file1 mtime1
		//path_to_file2 mtime2
		//...
		ifstream mtimes_file(mtimes_file_path.c_str());
		string path;
		time_t mtime;
		//mtimes_file >> path >> mtime;
		//cout << endl << "path: " << path << endl << "mtime: " << mtime;
		while(mtimes_file >> path >> mtime)
		{
			// Check if the file is in the vector
			bool found = false;
			for(int j=0; j<user_files->size(); j++)
			{
				if((*user_files)[j].get_path() == path){
					found = true;
					break;
				}
			}
			if(!found)
			{
				File_server new_file(path, mtime);
				user_files->push_back(new_file);
			}
		}
	}
	mtimes_file.close();

	// Unlock mutex
	pthread_mutex_unlock(user_files_mutex);
}
