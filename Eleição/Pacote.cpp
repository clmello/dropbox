#include "Pacote.h"

int ID;
int liderID;
int numPortas = 3;
int porta[10] = { 11111, 22222, 33333, 44444,55555,66666,77777,88888,99999,00000 };

Pacote::Pacote()
{
}

Pacote::Pacote(int sourceID, int destinID, string msg)
{
	this->sourceID = sourceID;
	this->destinID = destinID;
	this->msg = msg;
}
