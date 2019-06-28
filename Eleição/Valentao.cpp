#include "Valentao.h"

int contMSG = 1;
bool Lider = false;

void Valentao::lider(Pacote pacote) // Servidor chama se (pacote.sourceID != ID) && (buffer.destinID ==liderID)
{
	//Cliente cliente;
	
	if ("Eleicao" == pacote.msg)
	{
		if (pacote.destinID > pacote.sourceID)
		{
			cout << "Process " << pacote.sourceID << ", voce eh fraco. Eu sou o lider!\n";
			Pacote send(pacote.destinID, pacote.sourceID, "Fraco");

			Cliente::sendToOthers(send);

			eleicao(pacote.destinID);
		}

		else
		{
			Lider = false;
			sleep(1500);
		}
	}

	else if ("Lider" == pacote.msg)
	{
		cout << "Enviar par o lider\n";
		liderID = pacote.sourceID; // ID do lider atual
		Pacote send(pacote.sourceID, liderID, "Message " + contMSG++);
		Cliente::sendToLider(send);
	}
	else
	{
		cout << "===================";
		cout << pacote.msg << endl;
		cout << "Enviado por " << pacote.sourceID << endl;
		cout << "===================";

		Pacote send = Pacote(liderID, pacote.sourceID, "ACK:\n" + pacote.msg);
		Cliente::sendToOthers(send);
	}

}
void Valentao::resto(Pacote pacote) // Servidor chama se (pacote.sourceID !=ID) && (buffer.destinID != liderID)
{
		//Cliente cliente;

	if ("Eleicao" == pacote.msg)
	{
		if (pacote.destinID > pacote.sourceID)
		{
			cout << "Process " << pacote.sourceID << ", voce eh fraco. Eu sou o lider!\n";
			Pacote send = Pacote(pacote.destinID, pacote.sourceID, "Fraco");

			Cliente::sendToOthers(send);

			eleicao(pacote.destinID);
		}
	}
	else if ("Fraco" == pacote.msg)
	{
		cout << "OK process " << pacote.sourceID << ". Voce eh o lider!\n";
		Lider = false;
		sleep(3000);
	}

	else if ("Lider" == pacote.msg)
	{
		liderID = pacote.sourceID;
	}
	else
	{
		switch (pacote.destinID)
		{
		case 1:
			sleep(2001);
			break;
		case 2:
			sleep(2555);
			break;
		case 3:
			sleep(3507);
			break;
		case 4:
			sleep(1602);
			break;
		}

		cout << "===================";
		cout << pacote.msg << endl;
		cout << "Enviado pelo lider " << pacote.sourceID << endl;
		cout << "===================";

		Pacote send = Pacote(pacote.destinID, liderID, "Message " + contMSG++);
		Cliente::sendToLider(send);

	}
}
 void Valentao::eleicao(int sourceID)
{
		//Cliente cliente;

	Lider = true;
	//cout<<"Eleicao "<<sourceID<<endl;

	for (int i = sourceID + 1; i < numPortas; i++) //ID � o identificador do processo e numPortas = num servidores
	{
		Pacote pktEleicao(sourceID, i, "Eleicao");
		Cliente::sendToOthers(pktEleicao);

	//	cout<<"Process "<<i<<" comunicado\n";

		sleep(300);
		if (!Lider)
		{
			sleep(1700);
			break;
		}
	}

	if (Lider)
	{
		liderID = sourceID;
		cout << "\nEu sou o lider! Processo "<<sourceID<<endl;;

		for (int i = 1; i <= numPortas; i++)
		{
			if (sourceID != i)
			{
				Pacote pktEleicao =Pacote(sourceID, i, "Lider");
				Cliente::sendToOthers(pktEleicao);
			}
		}
	}
	else
	{
		Pacote pktEleicao = Pacote(sourceID, liderID, "Message " + contMSG++);
		Cliente::sendToLider(pktEleicao);

	}
}