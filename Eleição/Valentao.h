#ifndef BULLY
#define BULLY
#include <iostream>
#include "Pacote.h"
#include "Cliente.h"

using namespace std;

extern int contMSG;
extern bool Lider;
//Cliente cliente;


class Valentao {
public:
//	Cliente cliente;

	
	static void lider(Pacote pacote);
	static void resto(Pacote pacote);
	static void eleicao(int sourceID);


};

#endif
