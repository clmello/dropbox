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
}

void Communication_server::receive_header(int sockfd, struct packet *header)
{
	char *buffer = (char*)malloc(header_size);
	int bytes_received=0;
    while(bytes_received < header_size)
    {
        /* read from the socket */
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
    free(buffer);
}

// The type variable defines the type of output:
// 0 -> just return 0 (the pkt will be used by the caller)
// 1 -> command (int)
// 2 -> mtime (time_t)
// Since time_t is a signed integer that can be 32 or 64 bits long (depending on
//the system), we return a long int (64 bits signed integer)
long int Communication_server::receive_payload(int sockfd, struct packet *pkt, int type)
{
	char *buffer = (char*)malloc(max_payload);
    receive_header(sockfd, pkt);
	int bytes_received=0;
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, &buffer[bytes_received], pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");

        bytes_received+=n;
	}
	pkt->_payload = (const char*)buffer;
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

void *Communication_server::receive_commands(int sockfd, string username, int *thread_finished, vector<File_server> *user_files, pthread_mutex_t *user_files_mutex)//, vector<Connected_client> *connected_clients)
{
    bool close_thread = false;
    while(!close_thread) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        // Wait for a command
        cout << endl << sockfd << ": waiting for command";
        struct packet pkt;
        int command = receive_payload(sockfd, &pkt, 1);


		// Here, after every command the server sends a code (int) to the user:
		//  1: everything OK. The command will be executed;
		// -1: server is closing. The command will not be executed;
		//TODO: NÃO MAIS!!! ^ ISSO IA DAR PROBLEMA! NÃO TEM O FILE_NOT_FOUND!
        if(closing_server){
            send_int(sockfd, -1);
            command = 7;
        }
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

                // Receive mtime
                time_t mtime = receive_payload(sockfd, &pkt, 2);

				// This function locks the file mutex as a writer
				start_writing_file(path, user_files, user_files_mutex, mtime);

                // Receive the file
                receive_file(sockfd, path);

				// This function unlocks the mutex as a writer
				done_writing_file(path, user_files, user_files_mutex);

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

				// This function locks the file mutex as a reader
				int return_value = start_reading_file(path, user_files, user_files_mutex);

				// If the file doesn't exist, don't try to read it and tell the client that it doesn't exist
				// Send 1 if OK, send -1 if file doesn't exist
				if(return_value == -1){
					send_int(sockfd, -1);
					break;
				}
				else
					send_int(sockfd, 1);

                //Send mtime
                time_t mtime = get_mtime(filename, username, user_files, user_files_mutex);
                send_mtime(sockfd, mtime);

                // Send file
                send_file(sockfd, path);

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

                break;
            }
            case 4: // List server
            {
                cout << endl << sockfd << ": command 4 received";

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
            default:
            {
            }
        }
	}
}

void *Communication_server::receive_commands_helper(void* void_args)
{
    th_args* args = (th_args*)void_args;
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd, *args->username, args->thread_finished, args->user_files, args->user_files_mutex);
    return 0;
}

void Communication_server::send_string(int sockfd, string str)
{
    char* buffer = (char*)malloc(packet_size);
    // If the string is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)str.size()/(float)max_payload;
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

        // Read pkt.length characters from the string
        //and save it to pkt._payload
        char str_buff[pkt.length];
        strcpy(str_buff, str.substr((i-1)*max_payload, pkt.length).c_str());
        pkt._payload = (const char*)&str_buff;

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
        //------------------------------------------------------------------------
    }
    //free(buffer);
}

void Communication_server::send_int(int sockfd, int number)
{
    char* buffer;
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

    int i;
    int total_bytes_sent = 0;
    char *file_buffer = (char*)malloc(max_payload);

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

        // Read pkt.length bytes from the file
        size_t bytes_read = fread(file_buffer, 1, pkt.length, fp);
        if(bytes_read != pkt.length)
            cout << "\nError reading from file \"" << path << "\"";

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
        //------------------------------------------------------------------------
    }
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

    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt._payload, sizeof(char), pkt.length, fp);
    if (bytes_written_to_file < pkt.length)
        cout << "\nERROR WRITING TO " << path << endl;

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
    }
    fclose(fp);
}

int Communication_server::create_folder(string path)
{
    DIR* dir = opendir(path.c_str());
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
		if((*user_files)[i].get_mtime() > 0)
		{
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
