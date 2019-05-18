#include "client.h"
#include "Communication_client.h"

#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
/* #include <boost/filesystem.hpp>
using namespace boost::filesystem;

g++ -o client client.cpp -lboost_system -> comando para boost funcionar */
Communication_client communication;

Client::Client(std::string username, std::string hostname, int port){
    this->username = username;
	this->hostname = hostname;
	this->port = port;
    this->isLogged = false;
    this->dir = " ";
    this->command = 0;
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

pthread_t* Client::getCheckFilesThread() {
    return &check_files_thread;
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

int Client::getCommand() {
    return this->command;
}

void Client::setCommand(int command) {
    this->command = command;
}

void Client::setRunning(bool running) {
    this->running = running;
}

void Client::printWatchedFies() {
    if(this->watched_files.size() > 0) {
        std::cout << "\n\nWATCHED FILES:\n";
        for(unsigned int i = 0; i < this->watched_files.size(); i++){
            std::cout << this->watched_files[i].name << std::endl;
        }
        std::cout << std::endl;
    } else
        std::cout << "\nThere are no watched files\n";
}

void *Client::check_files_loop() {
    // enquanto a thread está aberta
    while(this->running) {
        int err = 0;
    
        // If files are already being watched
        if(this->watched_files.size()>0)
        {
            DIR* dir = opendir(this->dir.c_str());
            if(dir == NULL)
                std::cerr << "\nERROR opening folder\n";

            // For each watched file
            for(unsigned int n = 0; n < this->watched_files.size(); n++)
            {
                // Looks for it in the folder
                bool found = false;
                for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; )
                {
                    std::string fileName = d_struct->d_name;
                    if(fileName == this->watched_files[n].name)
                    {
                        found = true;

                        std::string file_ = this->dir + "/" + d_struct->d_name;
                        struct stat fileattrib;
                        if(stat(file_.c_str(), &fileattrib) < 0)
                            std::cout << "\nstat error\n";

                        // Checks if file was changed
                        if(difftime(this->watched_files[n].mtime, fileattrib.st_mtime))
                        {
                            std::cout << "\n\nthe file " << d_struct->d_name << " has changed!\n It should be uploaded!\n";
                            this->watched_files[n].mtime = fileattrib.st_mtime;
                        }
                    }
                }
                // If file isn't found, then it has been deleted
                if(!found)
                {
                    std::cout << "\n\nthe file " << this->watched_files[n].name << " has been deleted!\n It should be deleted on the server!\n";
                    this->watched_files.erase(this->watched_files.begin()+n);
                }
                // Reset the position of the directory stream
                rewinddir(dir);
            }

            // Checks for new files
            rewinddir(dir);
            if(dir == NULL)
                std::cout << "\nERROR opening folder\n";

            // For every file in the folder
            for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; )
            {
                // Looks for the file in the vector
                bool found = false;
                if(d_struct->d_name[0] != '.')
                {
                    for(unsigned int n = 0; n < this->watched_files.size(); n++)
                    {
                        std::string fileName = d_struct->d_name;
                        if(fileName == this->watched_files[n].name)
                        {
                            found = true;
                            break;
                        }
                    }
                    if(!found)
                    {
                        std::cout << "\n\nthe file " << d_struct->d_name << " is new!\n It should be uploaded!\n";

                        // Start watching file
                        std::string file_ = this->dir + "/" + d_struct->d_name;
                        struct stat fileattrib;
                        if(stat(file_.c_str(), &fileattrib) < 0)
                            std::cout << "\nstat error\n";

                        file newFile;
                        newFile.name = d_struct->d_name;
                        newFile.mtime = fileattrib.st_mtime;
                        this->watched_files.push_back(newFile);
                    }
                }
            }
            closedir(dir);
        }
        // If no files are watched
        else
        {
            DIR* dir = opendir(this->dir.c_str());
            if(dir == NULL)
                std::cout << "\nerror opening folder\n";

            // Loops through files in sync_dir
            for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; )
            {
                // If there is a file
                if(d_struct->d_name[0] != '.')
                {
                    // Start watching file
                    std::string file_name = this->dir + "/" + d_struct->d_name;
                    struct stat fileattrib;
                    if(stat(file_name.c_str(), &fileattrib) < 0)
                        std::cout << "\nstat error\n";

                    file newFile;
                    newFile.name = d_struct->d_name;
                    newFile.mtime = fileattrib.st_mtime;
                    watched_files.push_back(newFile);

                    std::cout << "\n\nthe file " << d_struct->d_name << " is new!\n It should be uploaded!\n";
                }
            }
            closedir(dir);
        }
        //printWatchedFies();
        sleep(10);
    }   
}

void *Client::check_files_helper(void* context) {
    return ((Client *)context)->check_files_loop();
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
    std::string argument;

    std::cout << "Digite um dos comandos a seguir:\n\n";
    std::cout << " upload <path/filename.ext>\n";
    std::cout << " download <filename.ext>\n";
    std::cout << " delete <filename.ext>\n";
    std::cout << " list_server\n";
    std::cout << " list_client\n";
    std::cout << " get_sync_dir\n";
    std::cout << " exit\n";

    while(running) {
       std::getline(std::cin, input);
        command = input.substr(0, input.find(" "));
        input = input.substr(input.find(" ") + 1, input.length());

        if(command == "upload") {
            std::cout << "Upload " << input << "\n";
            communication.upload_command(1, input, this->dir);
            // metodo pra upload
        }
        else if(command == "download") {
            std::cout << "Download " << argument << "\n";
            //communication.send_command(2);
            // metodo pra download
        }
        else if(command == "delete") {
            std::cout << "Delete " << argument << "\n";
/*
            if (argument == "")
                std::cout << "Delete needs argument <file>";

            else
            {
                std::string fileName = dir + "/" + argument;

                if (unlink(fileName.c_str()) == -1)
                    std::cout << "Error on Delete file: " << fileName << "\n";

                else
                {
                    std::cout << "Deleted file: " << fileName << "\n";
                }
            }
*/
            //communication.send_command(3);
        }
        else if(command == "list_server") {
            std::cout << "List Server \n";
            communication.send_command(4);
            // metodo pra list_server
        }
        else if(command == "list_client") {
            std::cout << "List Client \n";
            //communication.send_command(5); // NÃO PRECISA ENVIAR ESSE COMANDO PARA O SERVIDOR
             DIR *fileDir; 
            struct dirent *lsdir;

            fileDir= opendir(dir.c_str());

            while ((lsdir = readdir(fileDir)) != NULL)
            {
                if(lsdir->d_name[0] != '.') // Ignora . e ..
                    printf("%s\n", lsdir->d_name);
            }

            closedir(fileDir);
        }
        else if(command == "get_sync_dir") {
            std::cout << "Get Sync Dir \n";
            //communication.send_command(6);
            // metodo pra get_sync_dir
        }
        else if(command == "exit") {
            communication.send_command(7);
            running = false;
            pthread_join(this->check_files_thread, NULL);
            // metodo pra exit
        }
        else {
            std::cout << "Comando não está entre as opções válidas\n";
        }
    }
}

/*
Communication_client* Client::getCommunication() {
    return &communication;
}
*/


int main(int argc, char **argv) {
	if(argc < 4) {
		std::cerr << "Argumentos insuficiente, siga o formato a seguir:\n";
		std::cerr << "./dropboxClient <username> <server_ip_address> <port> \n";
		std::exit(1);
	}

	std::string username = argv[1];
   	std::string host = argv[2];
   	int port = atoi(argv[3]); // de repente pode ser um uint16?

	Client client = Client(username, host, port);

	/*** CONEXÃO COM O SERVIDOR ***/
    // Voltar pra essa questão de communication ser um argumento de client:
	// bool connected = client.communication.connect_client_server(client);
	bool connected = communication.connect_client_server(client);

    if(!connected) {
        std::cerr << "ERROR, can't connect with server \n";
        std::exit(1);
    }

    /*** SINCRONIZAÇÃO COM O SERVIDOR ***/
    // Aqui verifica se já existe o diretório, se não existe então cria
    std::string dir = client.createSyncDir();
	client.setDir(dir);

    client.setRunning(true);

    // chama método queinicializa thread que fica verificando se arquivos foram modificados
    pthread_create(client.getCheckFilesThread(), NULL, &Client::check_files_helper, &client);

    printf("Well, hi\n");
    client.userInterface();
}
