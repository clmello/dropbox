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

    bool running;
    struct file{time_t mtime; std::string name;};
    std::vector<file> watched_files;

    struct th_args{
		void* obj = NULL;
    };

public:
    Client(std::string username, std::string hostname, int port);

    // gets e sets
    void setUsername(std::string username);
    std::string getUsername();
	std::string getHostname();	
	int getPort();
    void setIsLogged(bool isLogged);
    bool getIsLogged();
    void setDir(std::string dir);
    void setRunning(bool running);

    // rest
    void *printWatchedFies();
    void *check_files_loop();
    static void *check_files_helper(void* void_args);
    std::string createSyncDir();
    void *initSyncClientThread();
    void userInterface();
};

#endif //CLIENT_H
