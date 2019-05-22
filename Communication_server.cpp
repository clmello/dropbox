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
        int n = read(sockfd, buffer, header_size-bytes_received);
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

int Communication_server::receive_payload(int sockfd, struct packet *pkt, bool is_command)
{
	char *buffer = (char*)malloc(max_payload);
    //cout << "\n\n" << sockfd << ": ENTREI NO RECEIVE_PAYLOAD\n\n";
    receive_header(sockfd, pkt);
	int bytes_received=0;
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		//cout << "\n" << sockfd << ": bytes lidos: "<<bytes_received<<endl;
	}
	//cout << "\n" << sockfd << ": bytes lidos: "<<bytes_received;
	pkt->_payload = (const char*)buffer;
	if(pkt->type != 1){ // If the packet is not a command
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
	return 0;
}

void *Communication_server::receive_commands(int sockfd, string username, int *thread_finished)//, vector<Connected_client> *connected_clients)
{
    bool close_thread = false;
    while(!close_thread) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        // Wait for a command
        cout << endl << sockfd << ": waiting for command";
        struct packet pkt;
        int command = receive_payload(sockfd, &pkt, true);
        
        
        
        // If server is trying to close, send -1 to the client. Otherwise, send 1
        //to signal that the command was received
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
                receive_payload(sockfd, &pkt, false);
                string filename = pkt._payload;
                path = path + "/server_sync_dir_" + username + "/" + filename;
                //cout << "String path: " << path << endl;
                
                // Receive mtime
                receive_payload(sockfd, &pkt, false);
                time_t mtime = *(time_t*)pkt._payload;
                //cout << "mtime: " << mtime << endl;
                
                update_watched_file(filename, mtime);
                
                // Receive the file
                receive_file(sockfd, path);
                
                break;
            }
            case 2: // Download from server
            {
                cout << endl << sockfd << ": command 2 received";
                
                string path = getenv("HOME");
                // Receive the file name
                receive_payload(sockfd, &pkt, false);
                string filename = pkt._payload;
                path = path + "/server_sync_dir_" + username + "/" + filename;
                cout << "\npath: " << path;
                
                //Send mtime
                time_t mtime = get_mtime(filename);
                char* mtime_char = (char*)&mtime;
                send_string(sockfd, mtime_char);
                
                // Send file
                send_file(sockfd, path);
                
                break;
            }
            case 3: // Delete file
            {
                cout << endl << sockfd << ": command 2 received";
                
                string path = getenv("HOME");
                receive_payload(sockfd, &pkt, false);
                string filename = pkt._payload;
                path = path + "/server_sync_dir_" + username + "/" + filename;
                cout << "String path: " << path;
                delete_file(path);
                remove_watched_file(filename);
                
                break;
            }
            case 4: // List server
            {
                cout << endl << sockfd << ": command 4 received";
		        
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
                
                send_string(sockfd, return_str);
        
                break;
            }
            case 6: // Get sync_dir
            {
                cout << endl << sockfd << ": command 6 received";
                
                // Open sync_dir folder
                string path = getenv("HOME");
                path = path + "/server_sync_dir_" + username;
                DIR *fileDir;
                struct dirent *lsdir;
                fileDir = opendir(path.c_str());
                
                // Get the number of files
                int number_of_files = 0;
                while ((lsdir = readdir(fileDir)) != NULL){
                    if(lsdir->d_name[0] != '.')
                        number_of_files++;
                }
                rewinddir(fileDir);
                
                // Send number of files to the client
                string number_of_files_str = to_string(number_of_files);
                //cout << "\nnumber of files: " << number_of_files_str << endl;
                send_string(sockfd, number_of_files_str);
                
                // Send the name of each file and its mtime
                while ((lsdir = readdir(fileDir)) != NULL){
                    if(lsdir->d_name[0] != '.'){
                        // Send filename
                        send_string(sockfd, lsdir->d_name);
                        
                        // Get mtime
                        time_t mtime = get_mtime(lsdir->d_name);
                        char* mtime_char = (char*)&mtime;
                        // Send mtime
                        send_string(sockfd, mtime_char);
                    }
                }
                closedir(fileDir);
                
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
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd, *args->username, args->thread_finished);
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
    cout << "\n\ntotal size: " << total_size;
    
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
        
        cout << "\nbytes read: " << bytes_read;
        cout << "\nConteudo lido: ";
	    printf("%.*s\n", max_payload, file_buffer);
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
        cout << "PACKET!\n";
        cout << "\npayload(char*): ";
        printf("%.*s\n", max_payload, pkt._payload);
        cout << "bytes sent: " << bytes_sent << endl;
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
    receive_payload(sockfd, &pkt, false);
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
        receive_payload(sockfd, &pkt, false);
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
}















