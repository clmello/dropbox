#include <string>

class Client {
private:
    std::string username; // userId
    bool isLogged;

public:
    Client(std::string username);

    void setUsername(std::string username);
    std::string getUsername();
    void setIsLogged(bool isLogged);
    bool getIsLogged();
    void userInterface();
};

