#include "Communication_server.h"

using namespace std;

Communication_server::Communication_server(int port)
{
	this->port = port;
	this->header_size = 10;
	this->max_payload = 502;
	this->packet_size = this->header_size + this->max_payload;
	this->buffer = (char*)malloc(packet_size);
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
		//create_folder("/home/"+connected_clients[connected_clients.size()-1].get_username()+"_syncdir");

        pthread_create(&client_thread, NULL, receive_commands_helper, &args);
        pthread_join(client_thread,NULL);
    //}

}

packet* Communication_server::receive_header(int sockfd)
{
	int bytes_received=0;
    bzero(buffer, header_size);
	cout << "\nbytes lidos: "<<bytes_received<<endl;
    while(bytes_received < header_size)
    {
        cout << "\n\nsockfd = " << sockfd << "\n\n";
        /* read from the socket */
        int n = read(sockfd, buffer, header_size);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	// Bytes from buffer[4] to buffer[7] are the size of _payload
	struct packet* header;
	header = (packet*)malloc(header_size);
	memcpy(&header->type, &buffer, 2);
	memcpy(&header->seqn, &buffer[2], 2);
	memcpy(&header->total_size, &buffer[4], 4);
	memcpy(&header->length, &buffer[8], 2);
	cout << "\n\ntype: " << header->type << endl;
	cout << "\n\nseqn: " << header->seqn << endl;
	cout << "\n\ntotal_size: " << header->total_size << endl;
	cout << "\n\npayload_size: " << header->length << endl << endl;
	
	return header;
}

packet* Communication_server::receive_payload(int sockfd)
{
    struct packet *pkt = receive_header(sockfd);
	int bytes_received=0;
    bzero(buffer, pkt->length);
    while(bytes_received < pkt->length)
    {
        // read from the socket
        int n = read(sockfd, buffer, pkt->length-bytes_received);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_received+=n;
		cout << "\nbytes lidos: "<<bytes_received<<endl;
	}
	pkt->_payload = (const char*)buffer;
	cout << "\npayload: " << *pkt->_payload << endl;
	
	return pkt;
}

void *Communication_server::receive_commands(int sockfd)
{
    //while(true) // TODO: ENQUANTO USUARIO NÃO FECHA
    //{
        // Wait for a command
        cout << "\nwaiting for command\n";
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
                string path = "/home/" + username + "_syncdir/" + receive_payload(sockfd)->_payload;
                char* file = receive_data(sockfd);
                long file_size = strlen(file);
                create_file(path, file, file_size);
                
            }
            case 2: // Download from server
            {
            }
            case 3: // Delete file
            {
            }
            case 4: // List server
            {
            }
            case 6: // Get sync_dir
            {
            }
            case 7: // Exit
            {
            }
            default:
            {
            }
        }
	//}
}

void *Communication_server::receive_commands_helper(void* void_args)
{
    th_args* args = (th_args*)void_args;
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd);
    return 0;
}

void Communication_server::send_data(int sockfd, uint16_t type, char* _payload, long total_payload_size, bool sending_file)
{
    // If a file is being sent, keep the file pointer
    FILE *fp;
    if(sending_file)
        fp = (FILE*)_payload;
    if(total_payload_size > max_payload)
    {
        // If the data is too large to send in one go, divide it into separate packets.
        float total_size_f = (float)total_payload_size/(float)max_payload;
        int total_size = total_size_f;
        if (total_size_f > total_size)
            total_size ++;
        
        int i;
        int total_bytes_sent = 0;
        for(i=0; i<total_size; i++)
        {
            // Create the packet that will be sent
            struct packet pkt;
            pkt.type = type;
            pkt.seqn = 0;
            pkt.total_size = total_size;
            if(max_payload > total_payload_size-total_bytes_sent)
                pkt.length = total_payload_size - total_bytes_sent;
            else
                pkt.length = max_payload;
	        pkt._payload = _payload;
	        
	        // copy pkt to buffer
	        buffer = (char*)&pkt;
	        
	        //------------------------------------------------------------------------
	        // SEND HEADER
            //------------------------------------------------------------------------
	        /* write in the socket */
	        int bytes_sent = 0;
	        while (bytes_sent < header_size)
	        {
	            int n = write(sockfd, &buffer[bytes_sent], header_size-bytes_sent);
                if (n < 0) 
		            printf("ERROR writing to socket\n");
		        bytes_sent += n;
            }
            total_bytes_sent += bytes_sent;
            cout << "bytes sent: " << bytes_sent << endl;
            
            //------------------------------------------------------------------------
	        // SEND PAYLOAD
            //------------------------------------------------------------------------
	        /* write in the socket */
	        bytes_sent = 0;
	        while (bytes_sent < pkt.length)
	        {
	            int n = write(sockfd, &pkt._payload[bytes_sent], pkt.length-bytes_sent);
                if (n < 0) 
		            printf("ERROR writing to socket\n");
		        bytes_sent += n;
            }
            total_bytes_sent += bytes_sent;
            cout << "bytes sent: " << bytes_sent << endl;
            //------------------------------------------------------------------------
        }
        // Increment the pointer so that we don't send the same data twice
        _payload += total_bytes_sent;
    }
    else // If the server can send it in one go
    {
        // Create the packet that will be sent
        struct packet pkt;
        pkt.type = type;
        pkt.seqn = 0;
        pkt.total_size = 1;
        pkt.length = total_payload_size;
	    pkt._payload = _payload;
	    
	    // copy pkt to buffer
	    buffer = (char*)&pkt;
	    
	    //------------------------------------------------------------------------
	    // SEND HEADER
        //------------------------------------------------------------------------
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
        
        //------------------------------------------------------------------------
	    // SEND PAYLOAD
        //------------------------------------------------------------------------
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
    }
    // If a file has been sent, close the file
    if(sending_file)
        fclose(fp);
}

char* Communication_server::receive_data(int sockfd)
{
    struct packet* pkt = receive_payload(sockfd);
    if(pkt->total_size == 1)
    {
        cout << "\n\ndata received: " << pkt->_payload << endl;
        return (char*)pkt->_payload;
    }
    else
    {
        char* data = (char*)malloc(pkt->length);
        memcpy(&data[strlen(data)], pkt->_payload, pkt->length);
        int i;
        uint32_t total_size = pkt->total_size;
        // Receive all the packets sent by the client
        // The first packet has already been received
        for(i=pkt->seqn+1; i<=total_size; i++)
        {
            pkt = receive_payload(sockfd);
            memcpy(&data[strlen(data)], pkt->_payload, pkt->length);
            cout << "\npacote " << pkt->seqn << " de " << pkt->total_size << " recebido";
            cout << "\ni: " << i;
        }
        cout << "\n\ndata received: " << data << endl;
        return data;
    }
}

char* Communication_server::read_file(string path)
{
    char ch, file_name[25];
    FILE *fp;
    
    fp = fopen(path.c_str(), "r"); // read mode
    
    if(fp == NULL)
        cout << "Error opening file " << path << endl;
    
    return (char*)fp;
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
    return 0;
}

int Communication_server::create_file(string path, char* file, long file_size)
{
    FILE* fp = fopen(path.c_str(), "w");
    
    int i;
    for(i=0; i<file_size; i++)
        fwrite(&file[i], 1, 8, fp);
        
    fclose(fp);
}

long Communication_server::get_file_size(string path)
{
    FILE* fp = fopen(path.c_str(), "r");
    if(fp==NULL)
        cout << "\nError opening file " << path << endl;
    fseek(fp, 0 , SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0 , SEEK_SET);
    
    return size;
}
