#include "Communication_server.h"

using namespace std;

Communication_server::Communication_server(int port)
{
	this->port = port;
	this->header_size = 10;
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

        struct th_args args;
        args.obj = this;
        args.newsockfd = &newsockfd;
		
		pthread_t client_thread;
		//Connected_client new_client();

        pthread_create(&client_thread, NULL, receive_commands_helper, &args);
        pthread_join(client_thread,NULL);
    //}

}

void Communication_server::receive_username(int sockfd)
{
	int bytes_lidos=0;
    bzero(buffer, 256+10);
	cout << "\nbytes lidos: "<<bytes_lidos<<endl;
    while(bytes_lidos<80) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        cout << "\n\nsockfd = " << sockfd << "\n\n";
        /* read from the socket */
        int n = read(sockfd, buffer, 256);
        if (n < 0)
            printf("ERROR reading from socket");
            
        bytes_lidos+=n;
		cout << "\nbytes lidos: "<<bytes_lidos<<endl;
		
		packet* pacote_recebido = (packet*)buffer;
		
		cout << "\n\npacote recebido: \ntipo: " << pacote_recebido->type;
		cout << "\nseqn: " << pacote_recebido->seqn;
		cout << "\ntotal_size: " << pacote_recebido->total_size;
		cout << "\nlength: " << pacote_recebido->length;
		cout << "\npayload: " << pacote_recebido->_payload <<endl <<endl;
	}
}

void *Communication_server::receive_commands(int newsockfd)
{
	int bytes_lidos=0;
    bzero(buffer, 256+header_size);
	cout << "\nbytes lidos: "<<bytes_lidos<<endl;
    while(bytes_lidos<80) // TODO: ENQUANTO USUARIO NÃO FECHA
    {
        cout << "\n\nnewsockfd = " << newsockfd << "\n\n";
        /* read from the socket */
        int n = read(newsockfd, buffer, 256+header_size-bytes_lidos);
        if (n < 0)
            printf("ERROR reading from socket");
        printf("Here is the message: %s\n", buffer);
        bytes_lidos+=n;
		cout << "\nbytes lidos: "<<bytes_lidos<<endl;
		packet* pacote_recebido = (packet*)buffer;
		cout << "\n\npacote recebido: \ntipo: " << pacote_recebido->type;
		cout << "\nseqn: " << pacote_recebido->seqn;
		cout << "\ntotal_size: " << pacote_recebido->total_size;
		cout << "\nlength: " << pacote_recebido->length;
		cout << "\npayload: " << pacote_recebido->_payload <<endl <<endl;
	}
	//packet* pacote_recebido = (packet*)buffer;
}

void *Communication_server::receive_commands_helper(void* void_args)
{
    th_args* args = (th_args*)void_args;
    ((Communication_server*)args->obj)->receive_commands(*args->newsockfd);
    return 0;
}
















