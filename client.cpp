#include "client.h"
#include "Communication_client.h"

#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
/* #include <boost/filesystem.hpp>
using namespace boost::filesystem;

g++ -o client client.cpp -lboost_system -> comando para boost funcionar */

Client::Client(std::string username, std::string hostname, int port){
    this->username = username;
	this->hostname = hostname;
	this->port = port;
    this->isLogged = false;
    this->dir = " ";
}

std::string Client::getUsername() {
    return this->username;
}

std::string Client::getHostname() {
	return this->hostname;
}

int Client::getPort() {
	return this->port;
}

void Client::setIsLogged(bool isLogged) {
    this->isLogged = isLogged;
}

bool Client::getIsLogged() {
    return this->isLogged;
}

void Client::setDir(std::string dir) {
    this->dir = dir;
}

// Pretendo mudar pra outra classe tipo um Folder ou Util
std::string Client::createSyncDir() {
    // filesystem path
    const char *homedir = getenv("HOME");
    std::string syncdir = "/sync_dir_" + getUsername();
    std::string dir = std::string(homedir) + syncdir;
    //std::string print;
    //std::cout << print << path;

	const char* folder = dir.c_str();
    struct stat info;
    if (stat(folder, &info) == 0 && S_ISDIR(info.st_mode)){
        std::cout << "Found folder." << std::endl;
    } else {
		// Read + Write + Execute:  S_IRWXU(user), S_IRWXG(group), S_IRWXO(others)
        mkdir(folder, S_IRWXU | S_IRWXG | S_IRWXO);
        std::cout << "Didn't found folder,\ncreating one:\n\t" << dir << std::endl;
    }
	return dir;
}

void Client::userInterface() {
    bool running = true;
    std::string input;
    std::string command;

    while(running) {
        std::getline(std::cin, input);
        command = input.substr(0, input.find(" "));

        if(command == "upload") {
            std::cout << "Upload \n";
            // metodo pra upload
        }
        else if(command == "download") {
            std::cout << "Download \n";
            // metodo pra download
        }
        else if(command == "delete") {
            std::cout << "Delete \n";
            // metodo pra delete
        }
        else if(command == "list_server") {
            std::cout << "List Server \n";
            // metodo pra list_server
        }
        else if(command == "list_client") {
            std::cout << "List Client \n";
            // metodo pra lit_client
        }
        else if(command == "get_sync_dir") {
            std::cout << "Get Sync Dir \n";
            // metodo pra get_sync_dir
        }
        else if(command == "exit") {
            running = false;
            // metodo pra exit
        }
        else {
            std::cout << "Comando não está entre as opções válidas";
        }
    }
}

int main(int argc, char **argv) {
	if(argc < 4) {
		// std::cerr for errors
		std::cerr << "Argumentos insuficiente, siga o formato a seguir:\n";
		std::cerr << "./dropboxClient <username> <server_ip_address> <port> \n";
		std::exit(1);
	}

	std::string username = argv[1];
   	std::string host = argv[2];
   	int port = atoi(argv[3]); // de repente pode ser um uint16?

	Client client = Client(username, host, port);

	/*** CONEXÃO COM O SERVIDOR ***/

	//// TA DANDO ERRO MAS NAO SEI
    Communication_client communication_client;
	bool connected = communication_client.connect_client_server(client);

	/*** SINCRONIZAÇÃO COM O SERVIDOR ***/
    // Aqui verifica se já existe o diretório, se não existe então cria
    std::string dir = client.createSyncDir();
	client.setDir(dir);

	//dai aqui sincroniza com o servidor


    /*
    se conseguir conectar e sincronizar entao ta logado,
    então aqui seria feito o
    client.setIsLogged(true)
    */

	// check_files -> pthread

    /*** INTERFACE COM O USUARIO ***/
    client.userInterface();
}
