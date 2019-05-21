#include "Communication_server.h"
#include "Synchronization_server.h"
#include <signal.h>
#include <unistd.h>

using namespace std;

void print_bytes(const void *object, size_t size)
{
  // This is for C++; in C just drop the static_cast<>() and assign.
  const unsigned char * const bytes = static_cast<const unsigned char *>(object);
  size_t i;

  printf("[ ");
  for(i = 0; i < size; i++)
  {
    printf("%02x ", bytes[i]);
  }
  printf("]\n");
}

// This global variable tells all the threads in the server that the server will close
bool closing_server = false;

// This function will handle the sigInt signal
void signal_handler(int sig)
{
	cout << "\nCaught signal " << sig << " (SIGINT)\nFinishing up . . .\n";
	closing_server = true;
}



int main()
{	
	// Link the signal_handler function to the sigInt signal. Once the user presses
	//ctrl+c, the function will be called and close_server will be true
	struct sigaction sigInt_handler;
	sigInt_handler.sa_handler = signal_handler;
	sigemptyset(&sigInt_handler.sa_mask);
	sigInt_handler.sa_flags = 0;
	sigaction(SIGINT, &sigInt_handler, NULL);
	
	// Start the server
	Synchronization_server server(4001);
	return 0;
}

