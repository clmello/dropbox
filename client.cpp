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
#include <sstream>
/* #include <boost/filesystem.hpp>
using namespace boost::filesystem;

g++ -o client client.cpp -lboost_system -> comando para boost funcionar */
Communication_client communication;

bool running;

pthread_mutex_t socket_mtx;
pthread_mutex_t watched_files_copy_mtx;
std::vector<Client::file> watched_files_copy;

Client::Client(std::string username, std::string hostname, int port, std::string download_path){
    this->username = username;
	this->hostname = hostname;
	this->port = port;
    this->isLogged = false;
    this->dir = " ";
    this->command = 0;
	this->download_path = download_path;
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

time_t Client::get_mtime(std::string filename) {
    for(int i=0; i < this->watched_files.size(); i++)
    {
        if(filename == this->watched_files[i].name)
            return this->watched_files[i].mtime;
    }
    return -1;
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

void Client::check_files() {
    int err = 0;

    // If files are already being watched
    if(this->watched_files.size()>0)
    {
        DIR* dir = opendir(this->dir.c_str());
        if(dir == NULL)
            std::cerr << "\nERROR opening folder\n";

        // For each watched file
        for(unsigned int n = 0; n < this->watched_files.size(); n++) {
            // Looks for it in the folder
            bool found = false;
            for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; ) {
                std::string fileName = d_struct->d_name;
                if(fileName == this->watched_files[n].name) {
                    found = true;

                    std::string file_ = this->dir + "/" + d_struct->d_name;
                    struct stat fileattrib;
                    if(stat(file_.c_str(), &fileattrib) < 0)
                        std::cout << "\nstat error\n";

                    // Checks if file was changed
                    if(difftime(this->watched_files[n].local_mtime, fileattrib.st_mtime)) {
                        std::cout << "\n\nthe file " << d_struct->d_name << " has changed!\n It should be uploaded!\n";
                        //std::cout << "\ntime changed: "
                        this->watched_files[n].local_mtime = fileattrib.st_mtime;
                        this->watched_files[n].mtime = fileattrib.st_mtime;
                    }
                }
            }
            // If file isn't found, then it has been deleted
            if(!found) {
                std::cout << "\n\nthe file " << this->watched_files[n].name << " has been deleted!\n It should be deleted on the server!\n";
                this->watched_files[n].mtime = -1;
            }
            // Reset the position of the directory stream
            rewinddir(dir);
        }

        // Checks for new files
        rewinddir(dir);
        if(dir == NULL)
            std::cout << "\nERROR opening folder\n";

        // For every file in the folder
        for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; ) {
            // Looks for the file in the vector
            bool found = false;
            if(d_struct->d_name[0] != '.' && (int)d_struct->d_name[0] > 33 && (int)d_struct->d_name[0] < 122) {
                for(unsigned int n = 0; n < this->watched_files.size(); n++) {
                    std::string fileName = d_struct->d_name;
                    if(fileName == this->watched_files[n].name) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    std::cout << "\n\nthe file " << d_struct->d_name << " is new!\n It should be uploaded!\n";

                    // Start watching file
                    std::string file_ = this->dir + "/" + d_struct->d_name;
                    struct stat fileattrib;
                    if(stat(file_.c_str(), &fileattrib) < 0)
                        std::cout << "\nstat error\n";

                    file newFile;
                    newFile.name = d_struct->d_name;
                    newFile.local_mtime = fileattrib.st_mtime;
                    newFile.mtime = 0;
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
                newFile.local_mtime = fileattrib.st_mtime;
                newFile.mtime = 0;
                this->watched_files.push_back(newFile);
                std::cout << "\nwatched_files[0].name: " << watched_files[0].name;
                std::cout << "\nwatched_files[0].mtime: " << watched_files[0].mtime;

                std::cout << "\n\nthe file " << d_struct->d_name << " is new!\n It should be uploaded!\n";
            }
        }
        closedir(dir);
    }
}

void *Client::check_files_loop() {
    // enquanto a thread está aberta
    while(running) {
        check_files();
        communication.get_sync_dir(6, &watched_files, this->dir);
        pthread_mutex_lock(&watched_files_copy_mtx);
        watched_files_copy = watched_files;
        pthread_mutex_unlock(&watched_files_copy_mtx);
        sleep(10);
    }
}

void *Client::check_files_helper(void* context) {
    return ((Client *)context)->check_files_loop();
}

void Client::copy_file(std::string original_path, std::string copy_path) {
    std::cout << "\nOriginal Path: " << original_path;
    std::cout << "\nCopy Path: " << copy_path;

    std::ifstream  src(original_path.c_str(), std::ios::binary);
    std::ofstream  dst(copy_path.c_str(),   std::ios::binary);

    dst << src.rdbuf();
}

bool Client::file_exists(std::string path, std::string filename) {
    //std::cout << "\nvai verificar se o file existe";
   // std::cout << "\npath na file_exists: " << path;
    DIR *fileDir;
    struct dirent *lsdir;
    fileDir= opendir(path.c_str());

    while ((lsdir = readdir(fileDir)) != NULL)
    {
        if(lsdir->d_name[0] != '.') { // Ignora . e ..
            //std::cout << "\nNome do arquivo do diretório: " << lsdir->d_name;
            if(lsdir->d_name == filename) {
                printf("%s\n", lsdir->d_name);
                return true;
            }
        }
    }
    //std::cout << "\nNão achou";
    return false;
}

void Client::get_sync_dir_client() {
    // cria o diretorio
    createSyncDir();

    // inicializa as atividades de sincronização
    //communication.get_sync_dir(6, watched_files, this->dir);
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

void Client::remove_from_watched_files(std::string filename) {
    for(int i=0; i < watched_files.size(); i++)
    {
        if(filename == watched_files[i].name)
            //std::cout << "\n\nwatched_file: "<< watched_files[i].name << "\tfilename: " << filename;
            watched_files.erase(watched_files.begin()+i);
    }
}


void Client::userInterface() {
    std::string input;
    std::string command;

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
        command = "";
        command = input.substr(0, input.find(" "));
        input = input.substr(input.find(" ") + 1, input.length());

        if(command == "upload") {
            std::cout << "Upload " << input << "\n";

            std::string filename = input.substr(input.find_last_of("\\/")+1, input.length());
            // nesse caso o input é o path!
            input = input.substr(0, input.find_last_of("\\/"));
            std::cout << "\n!!!filename: " << filename << std::endl;
            std::cout << "!!!path: " << input << std::endl;

            bool fileExists = file_exists(input, filename);
            if(fileExists) {
                std::string original_path = input + '/' + filename;
                std::string copy_path = this->dir + '/' + filename;
                copy_file(original_path, copy_path);

                /* DEPENDENDO DA PRA VER SE COLOCA NA WATCHED_FILES OU DEIXA PRA CHECK_FILES FAZER ISSO */

                //std::cout << "\nVou entrar na mtime!";
                //time_t mtime = get_mtime(input);
                //std::cout << "\nmtime: " << mtime << "\n";
                //communication.upload_command(1, filename, input, mtime);
                //std::cout << "\nEnviou!\n";
            } else {
                std::cout << "\nNão foi possível enviar o arquivo porque ele não existe.\n";
            }
        }
        else if(command == "download") {
            std::cout << "\nDownload " << input << "\n";
			std::string path = getenv("HOME");
			//std::string path = this->download_path + '/' + input;
			path = path + '/' + input;       
			std::cout << "\npath: " << path;
            file downloadFile;
            communication.download_command(2, input, path, &downloadFile);

            if(downloadFile.mtime == -1) {
                std::cout << "\nCan't download file beacuse it doesn't exist at server.";
            } else {
                std::cout << "\nJá passei pela download_command e recebi de volta:";
                std::cout << "\nDownloaded file: " << downloadFile.name;
                std::cout << "\ndownloadFile.mtime" << downloadFile.mtime;
                this->watched_files.push_back(downloadFile);
            }
            // metodo pra download
        }
        else if(command == "delete") {
            std::cout << "\nDelete " << input << "\n";
            bool fileExists = file_exists(this->dir, input);
            if(fileExists) {
                /* do novo jeito a ideia é  deletar o arquivo do file e aí a check_files (?) que vai verificar que tem arquivo deletado e envia o comando*/
                //communication.delete_file(input);
                std::string path = this->dir + '/' + input;
                //std::cout << "\n!!!!DELETANDO O ARQUIVOOO: " << path;
                communication.delete_file(path);
                //communication.delete_command(3, input, this->dir);
                // removeu da pasta, agora remove dos watched files
                // precisa indicar que vai ser deletado no server, então não pode deletar
                //remove_from_watched_files(input);
            } else {
                std::cout << "\nNão foi possível deletar o arquivo porque ele não existe.\n";
            }
        }
        else if(command == "list_server") {
            std::cout << "List Server \n";
            communication.list_server_command(4);
            // metodo pra list_server
        }

        //
        else if(command == "list_client") {
        	pthread_mutex_lock(&watched_files_copy_mtx);
        	std::stringstream ls;
        	ls.str("");
        	for(int i=0; i<watched_files_copy.size(); i++)
        	{
		    	time_t t = watched_files[i].mtime;
			    struct tm lt;
			    localtime_r(&t, &lt);
			    char timebuf[80];
			    strftime(timebuf, sizeof(timebuf), "%c", &lt);
			    
        		ls << watched_files_copy[i].name << "(" << timebuf << ") ";
        	}
        	pthread_mutex_unlock(&watched_files_copy_mtx);
        	std::cout << std::endl << ls.str();
        }
        else if(command == "get_sync_dir") {
            std::cout << "Get Sync Dir \n";
            get_sync_dir_client();
            //communication.send_command(6);
            // metodo pra get_sync_dir
        }
        else if(command == "exit") {
            communication.exit_command(7);
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
   	int port = atoi(argv[3]);

	char cwd[1024];
    getcwd(cwd, sizeof(cwd));
	std::string s_cwd = cwd;
	std::cout << "\ns_cwd " << s_cwd;

	Client client = Client(username, host, port, s_cwd);

	/*** CONEXÃO COM O SERVIDOR ***/
    // Voltar pra essa questão de communication ser um argumento de client:
	// bool connected = client.communication.connect_client_server(client);
	bool connected = communication.connect_client_server(client);

    if(!connected) {
        std::cerr << "ERROR, can't connect with server \n";
        std::exit(1);
    }

    running = true;
    pthread_mutex_init(&socket_mtx, NULL);
    pthread_mutex_init(&watched_files_copy_mtx, NULL);

    // Cria diretório de sincronização
    std::string dir = client.createSyncDir();
	client.setDir(dir);

    /*** SINCRONIZAÇÃO COM O SERVIDOR ***/
    // Aqui verifica se já existe o diretório, se não existe então cria
    //client.syncClient();

    /* Inicializa thread de sincronização*/
    pthread_create(client.getCheckFilesThread(), NULL, &Client::check_files_helper, &client);

    client.userInterface();
}
