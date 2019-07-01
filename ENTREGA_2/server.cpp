//#include "Communication_server.h"
#include "Synchronization_server.h"
#include "backup.h"
#include <signal.h>
#include <unistd.h>

using namespace std;

// This global variable tells all the threads in the server that the server will close
bool closing_server = false;

// This function will handle the sigInt signal
void signal_handler(int sig)
{
	cout << "\nCaught signal " << sig << " (SIGINT)\nFinishing up . . .\n";
	closing_server = true;
}



int main(int argc, char **argv)
{
	string host = "";
	int port = 0;
	switch (argc) {
		case 1:{
			break;
		}
		case 3:{
			host = argv[1];
			port = atoi(argv[2]);
			break;
		}
		default:{
			std::cerr << "Wrong usage. Use the following format:\n";
			std::cerr << "./dropboxServer <active_server_ip_address> <active_server_port> \n";
			std::exit(1);
		}
	}

	// Link the signal_handler function to the sigInt signal. Once the user presses
	//ctrl+c, the function will be called and close_server will be true
	/*struct sigaction sigInt_handler;
	sigInt_handler.sa_handler = signal_handler;
	sigemptyset(&sigInt_handler.sa_mask);
	sigInt_handler.sa_flags = 0;
	sigaction(SIGINT, &sigInt_handler, NULL);*/
	// Also, ignore sigpipe
    signal(SIGPIPE, SIG_IGN);

	if(host=="")
	{
		// Start the server
		cout << "\n\nmain server\n\n";
		Synchronization_server server;
		server.Init(6000);
	}
	else
	{
		// Start the backup
		cout << "\n\nbackup server\n\n";
		Backup backup(host, port, port+50);
	}

	return 0;
}
