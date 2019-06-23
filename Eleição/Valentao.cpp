#include "Valentao.h"

VAlentao::lider(Pacote pacote) // Servidor chama se (pacote.sourceID != ID) && (buffer.destinID ==liderID)
    lider(pacote);
{
    if ("Eleicao" == pacote.msg)
    {
        if (pacote.destinID > pacote.sourceID)
        {
            cout << "Process " << pacote.sourceID << ", voce eh fraco. Eu sou o lider!\n";
            Pacote send = new Pacote(pacote.destinID, pacote.sourceID, "Fraco");

            //Cliente.sendToOthers(send);

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
        Pacote send = new Pacote(ID, liderID, "Message " + contMSG++);
        // Cliente.sendToLider(send);*/
    }
    else
    {
        cout << "===================";
        cout << pacote.msg << endl;
        cout << "Enviado por " << pacote.sourceID << endl;
        cout << "===================";

        Pacote send = new Pacote(liderID, pacote.sourceID, "ACK:\n" + pacote.message);
        //Cliente.sendToOthers(response);
    }

}
Valentao::resto(Pacote pacote) // Servidor chama se (pacote.sourceID !=ID) && (buffer.destinID != liderID)
{
    if ("Eleicao" == pacote.msg)
    {
        if (pacote.destinID > pacote.sourceID)
        {
            cout << "Process " << pacote.sourceID << ", voce eh fraco. Eu sou o lider!\n";
            Pacote send = new Pacote(pacote.destinID, pacote.sourceID, "Fraco");

            // Cliente.sendToOthers(send);

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

        Pacote send = new Pacote(pacote.destinID, liderID, "Message " + contMSG++);
        // Cliente.sendToLider(response);
    }
}
Valentao::eleicao(int sourceID)
{
    Lider = true;

    for (int i = ID + 1; i <= numPortas; i++) //ID é o identificador do processo e numPortas = num servidores
    {
        Pacote pktEleicao = new Pacote(sourceID, i, "Eleicao");
        //            Cliente.sendtoOthers(pktEleicao);

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
        cout << "\nEu sou o lider!\n";

        for (int i = 1; i <= numPortas; i++)
        {
            if (ID != i)
            {
                Pacote pktEleicao = new Pacote(sourceID, i, "Lider");
                //Cliente.sendToOthers(pktEleicao);
            }
        }
    }
    else
    {
        Pacote pktEleicao = new Pacote(ID, liderID, "Message " + contMSG++);
        // Cliente.sendToLider(send);
    }
}