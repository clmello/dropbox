#ifndef PKT
#define PKT
#include <string>

using std::string;

extern int ID;
extern int liderID;
extern int numPortas;
extern int porta[];



class Pacote {

public:
	int sourceID;
	int destinID;
	string msg;

	
	Pacote();
	Pacote(int sourceID, int destinID, string msg);

};
#endif
