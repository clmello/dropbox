#include "Communication_server.h"

using namespace std;

Communication_server::Communication_server(int port)
{
	this->port = port;
	this->header_size = 10;
	this->max_payload = 502;
	this->packet_size = this->header_size + this->max_payload;
	this->buffer = (char*)malloc(packet_size);
	this->header = (packet*)malloc(header_size);
	cout << "\nchamando aceita_conexoes\n";
	accept_connections();
}

void *Communication_server::accept_connections()
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    //while(true) // TODO: ENQUANTO USUARIO NÃO FECHA
    //{
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

        cout << "\nesperando conexao\n";
        clilen = sizeof(struct sockaddr_in);
        if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
            printf("ERROR on accept");

        cout << "CLIENTE DE IP " << cli_addr.sin_port << " CONECTADO!";

        // Create a struct with the arguments to be sent to the new thread
        struct th_args args;
        args.obj = this;
        args.newsockfd = &newsockfd;
		
		// Create the new connected client and add it to the connected_clients vector
		pthread_t client_thread;
		//TODO: VERIFICAR SE TEM DUAS CONEXOES
		Connected_client new_client(client_thread, receive_payload(newsockfd)->_payload, newsockfd);
		connected_clients.push_back(new_client);
		cout << "\nUsername: " << connected_clients[0].get_username() << endl;
		this->username = connected_clients[0].get_username();
		
		// Create client folder, if it doesn't already exist
		string homedir = getenv("HOME");
		create_folder(homedir+"/sync_dir_"+connected_clients[connected_clients.size()-1].get_username());

        // Create the thread for this client
        pthread_create(&client_thread, NULL, receive_commands_helper, &args);
    //}
    pthread_join(client_thread,NULL);

}

packet* Communication_server::receive_header(int sockfd)
{
    cout << "\n\nENTREI NO RECEIVE_HEADER\n\n";
	int bytes_received=0;
	cout << "\n\nbytes lidos: "<<bytes_received;
    while(bytes_received < header_size)
    {
        //cout << "\n\nsockfd = " << sockfd << "\n\n";
        /* read from the socket */
        cout << "\nBYTES_LIDOS ANTES DO READ: " << bytes_received;
        int n = read(sockfd, buffer, header_size-bytes_received);
        cout << "\nBYTES_LIDOS DEPOIS DO READ: " << bytes_received;
        cout << "\nN DEPOIS DO READ: " << n;
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		//cout << "\nbytes lidos: "<<bytes_received;
	}
	if(bytes_received != 0) // No need to copy anything to the header if no bytes were received
	{
	    // Bytes from buffer[4] to buffer[7] are the size of _payload
	    memcpy(&header->type, &buffer[0], 2);
	    memcpy(&header->seqn, &buffer[2], 2);
	    memcpy(&header->total_size, &buffer[4], 4);
	    memcpy(&header->length, &buffer[8], 2);
	    cout << "\ntype: " << header->type;
	    cout << "\nseqn: " << header->seqn;
	    cout << "\ntotal_size: " << header->total_size;
	    cout << "\npayload_size: " << header->length << endl;
    }
	
	return header;
}

packet* Communication_server::receive_payload(int sockfd)
{
    cout << "\n\nENTREI NO RECEIVE_PAYLOAD\n\n";
    struct packet *pkt = receive_header(sockfd);
	int bytes_received=0;
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		//cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	cout << "\nbytes lidos: "<<bytes_received;
	pkt->_payload = (const char*)buffer;
	/*if(pkt->type != 1){ // If the packet is not a command
	    cout << "\npayload(char*): ";
	    printf("%.*s\n", max_payload, pkt->_payload);
    }
    else{ // If the packet is a command
	    cout << "\npayload(int): ";
        int command;
        memcpy(&command, pkt->_payload, pkt->length);
        cout << command;
    }*/
    cout << endl << endl;
	return pkt;
}

void *Communication_server::receive_commands(int sockfd)
{
    bool _exit = false;
    while(!_exit) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        // Wait for a command
        cout << "\nwaiting for command\n";
        bzero(buffer, header_size);
        cout << "buffer[0]: " << buffer[0] << endl;
        struct packet *pkt = receive_payload(sockfd);
        while(pkt->length == 0)
        {
            pkt = receive_payload(sockfd);
        }
        int command;
        memcpy(&command, pkt->_payload, pkt->length);
        cout << "command received: " << command << endl;
        
        switch(command)
        {
            case 1: // Upload to server
            {
                cout << "\ncommand 1 received\n";
                
                string path = getenv("HOME");
                path = path + "/sync_dir_" + username + "/" + receive_payload(sockfd)->_payload;
                cout << "String path: " << path;
                receive_file(sockfd, path);
                
                break;
            }
            case 2: // Download from server
            {
                cout << "\ncommand 2 received\n";
                
                break;
            }
            case 3: // Delete file
            {
                cout << "\ncommand 2 received\n";
                
                string path = getenv("HOME");
                path = path + "/sync_dir_" + username;
                cout << "String path: " << path;
                delete_file(path);
                
                break;
            }
            case 4: // List server
            {
                cout << "\ncommand 4 received\n";
		        
                string path = getenv("HOME");
                path = path + "/sync_dir_" + username;
                DIR *fileDir;
                struct dirent *lsdir;

                fileDir = opendir(path.c_str());

                string return_str;
                while ((lsdir = readdir(fileDir)) != NULL)
                {
                    if(lsdir->d_name[0] != '.')
                        return_str = return_str + lsdir->d_name + " ";
                }
                return_str.pop_back();

                closedir(fileDir);
                
                send_string(sockfd, return_str);
        
                break;
            }
            case 6: // Get sync_dir
            {
                cout << "\ncommand 6 received\n";
                
                break;
            }
            case 7: // Exit
            {
                cout << "\ncommand 7 received\n";
                
                _exit = true;
                
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
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd);
    return 0;
}

void Communication_server::send_string(int sockfd, string str)
{
    // If the string is too large to send in one go, divide it into separate packets.
    // Get the number of packets necessary (total_size)
    float total_size_f = (float)str.size()/(float)max_payload;
    int total_size = total_size_f;
    if (total_size_f > total_size)
        total_size ++;
    cout << "\n\ntotal size: " << total_size;
    
    int i;
    int total_bytes_sent = 0;
    char *str_buff = (char*)malloc(str.size());
    cout << "\n\nenviando: " << endl << str;
    
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
        
        cout << endl << total_bytes_sent << " bytes have been sent";
        cout << endl << str.size() - (total_bytes_sent - header_size*(i-1)) << " bytes will be sent";
        
        // Read pkt.length characters from the string
        //and save it to pkt._payload
        strcpy(str_buff, str.substr((i-1)*max_payload, pkt.length).c_str());
        pkt._payload = str_buff;
        
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
        cout << "\n\nHEADER!\n";
        cout << "bytes sent: " << bytes_sent << endl;
        cout << "type: " << pkt.type;
        cout << "\nseqn: " << pkt.seqn;
        cout << "\ntotal_size: " << pkt.total_size;
        cout << "\npayload_size: " << pkt.length << endl;
        
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
    free(str_buff);
}

void Communication_server::send_file(int sockfd, string file_name, string path)
{        
    // To send a file, you must first send the file name, and then the file
    //------------------------------------------------------------------------
    // SEND FILENAME
    //------------------------------------------------------------------------
    //const char* payload = file_name.c_str();
    
    // Create the packet that will be sent
    struct packet pkt;
    pkt.type = 0;
    pkt.seqn = 1;
    pkt.total_size = 1;
    pkt.length = 9;
	pkt._payload = file_name.c_str();
    std::cout << "\n\nfilename: " << pkt._payload << std::endl;
    
	// copy pkt to buffer
	buffer = (char*)&pkt;
	
	// send header
	/* write in the socket */
	int bytes_sent = 0;
	while (bytes_sent < header_size)
	{
	    int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    
    //send payload
	/* write in the socket */
	bytes_sent = 0;
	while (bytes_sent < pkt.length)
	{
	    int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
        if (n < 0) 
		    printf("ERROR writing to socket\n");
		bytes_sent += n;
    }
    cout << "bytes sent: " << bytes_sent << endl;
    //------------------------------------------------------------------------
    
    //------------------------------------------------------------------------
    // SEND FILE
    //------------------------------------------------------------------------
    FILE *fp = fopen(path.c_str(), "r");
    if(fp == NULL)
        cout << "Error opening file " << path << endl;
    
    // Get the size of the file
    fseek(fp, 0 , SEEK_END);
    long total_payload_size = get_file_size(fp);
    
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
    cout << "\n\nenviando: " << endl;
	printf("%.*s\n", max_payload, buffer);
    
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
        
        cout << endl << total_bytes_sent << " bytes have been sent";
        cout << endl << total_payload_size - (total_bytes_sent - header_size*(i-1)) << " bytes will be sent";
        
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
            int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
            if (n < 0) 
	            printf("ERROR writing to socket\n");
	        bytes_sent += n;
        }
        total_bytes_sent += bytes_sent;
        cout << "\n\nHEADER!\n";
        cout << "bytes sent: " << bytes_sent << endl;
        cout << "type: " << pkt.type;
        cout << "\nseqn: " << pkt.seqn;
        cout << "\ntotal_size: " << pkt.total_size;
        cout << "\npayload_size: " << pkt.length << endl;
        
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
    free(file_buffer);
    fclose(fp);
}


void Communication_server::receive_file(int sockfd, string path) {
    FILE *fp = fopen(path.c_str(), "w");
    if(fp==NULL)
        cout << "\nERROR OPENING " << path << endl; 

    // Get the number of packets to be received
    // To do that, we must receive the first packet
    struct packet* pkt = receive_payload(sockfd);
    uint32_t total_size = pkt->total_size;
    cout << "\n\nTHE SERVER WILL RECEIVE " << total_size << " PACKETS!\n";
    // Write the first payload to the file
    ssize_t bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
    if (bytes_written_to_file < pkt->length)
        cout << "\nERROR WRITING TO " << path << endl;
    
    cout << bytes_written_to_file << " bytes written to file" << endl;
    
    // Receive all the [total_size] packets
    // It starts at 2 because the first packet has already been received
    int i;
    for(i=2; i<=total_size; i++)
    {
        // Receive payload
        pkt = receive_payload(sockfd);
        // Write it to the file
        bytes_written_to_file = fwrite(pkt->_payload, sizeof(char), pkt->length, fp);
        if (bytes_written_to_file < pkt->length)
            cout << "\nERROR WRITING TO " << path << endl;
        cout << "\n" << bytes_written_to_file << " bytes written to file\n";
    }
    fclose(fp);
}

int Communication_server::create_folder(string path)
{
    DIR* dir = opendir(path.c_str());
    cout << "\npath: " << path << endl;
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
        cout << "\ncommand: " << command << endl;
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
        cout << "Error deleting file";
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
