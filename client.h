#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client {
private:
    std::string username; // userId
	std::string hostname;	
	int port;    
	bool isLogged;
    std::string dir;

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

    // rest
    std::string createSyncDir();
    void userInterface();
};

#endif // COMMUNICATION_CLIENT_H
