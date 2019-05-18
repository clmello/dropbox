#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>

class Client {
private:
    std::string username; // userId
	std::string hostname;	
	int port;    
	bool isLogged;
    std::string dir;
    int command;

    bool running;
    struct file{time_t mtime; std::string name;};
    std::vector<file> watched_files;

    pthread_t check_files_thread;
    //Communication_client communication;

public:
    struct th_args{
		void* obj = NULL;
    };

    Client(std::string username, std::string hostname, int port);

    // gets e sets
    void setUsername(std::string username);
    std::string getUsername();
	std::string getHostname();	
	int getPort();
    void setIsLogged(bool isLogged);
    bool getIsLogged();
    void setDir(std::string dir);
    void setCommand(int command);
    int getCommand();
    void setRunning(bool running);
    pthread_t* getCheckFilesThread();
    //Communication_client* getCommunication();

    // rest
    void printWatchedFies();
    void *check_files_loop();
    static void *check_files_helper(void* void_args);
    std::string createSyncDir();
    //static void *user_interface_helper(void* void_args);
    void userInterface();
};

#endif //CLIENT_H
