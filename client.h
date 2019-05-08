#include "communication_client.h"

#include <string>

class Client {
private:
    std::string username; // userId
    bool isLogged;
    std::string dir;

public:
    Client(std::string username);

    // gets e sets
    void setUsername(std::string username);
    std::string getUsername();
    void setIsLogged(bool isLogged);
    bool getIsLogged();
    void setDir(std::string dir);

    // rest
    std::string createSyncDir();
    void userInterface();
};

