#include <iostream>
#include "Cliente.h"
#include "Servidor.h"
#include <pthread.h>

void *initCliente(void *arg);
void *initServ(void *arg);



int main(int argc, char *argv[])
{
	ID=atoi(argv[1]);
	cout << "Id: "<<ID;
//	cin >> ID;

	pthread_t th1,th2;
	
	//Servidor servidor;

	//Cliente cliente;
	pthread_create(&th1,NULL,initCliente,NULL);
	//cliente.run();

	pthread_create(&th2,NULL,initServ,NULL);	

	cout <<"Lider "<<liderID<<endl;

	return 0;
}

void *initCliente(void *arg)
{
	Cliente cliente;
		//cout<<"entrou initcli";

	cliente.run();
}

void *initServ(void *arg)
{
	Servidor servidor;
		//cout<<"entrou initserv";

}