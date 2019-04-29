#include "client.h"

#include <iostream>
using namespace std;

Client::Client(std::string username){
    this->username = username;
    this->isLogged = false;
}

std::string Client::getUsername() {
    return this->username;
}

void Client::setIsLogged(bool isLogged) {
    this->isLogged = isLogged;
}

bool Client::getIsLogged() {
    return this->isLogged;
}

void Client::userInterface() {
    bool running = true;
    std::string input;
    std::string command;

    while(running) {
        std::getline(std::cin, input);
        command = input.substr(0, input.find(" "));


        // SWITCH NAO ACEITA STRING, FAZ UM IF

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

	Client client = Client(username);

	// CONEXÃO COM O SERVIDOR

	// SINCRONIZAÇÃO COM O SERVIDOR

    /*
    se conseguir conectar e sincronizar entao ta logado,
    então aqui seria feito o
    client.setIsLogged(true)
    */

    // interface com o usuário
    client.userInterface();
}

