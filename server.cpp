#include "Communication_server.h"
#include <iostream>
#include <pthread.h>

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

int main()
{
	/*
	char buffer[10];
	
	struct packet pkt;
	pkt.type = 10;
	pkt.seqn = 3;
	pkt.total_size = 340;
	pkt. length = 6;
	pkt._payload = (const char*)"teste";
	//memcpy(pkt._payload, "teste", sizeof("teste"));
	
	
	memcpy(&buffer, &pkt, 10);
	print_bytes(&buffer, sizeof(buffer));
	
	uint16_t _payload_size;
	memcpy(&_payload_size, &buffer[8], 2);
	print_bytes(&_payload_size, sizeof(_payload_size));
	cout << "\npayload_size: " << _payload_size << endl;
	//cout << "\nsizeof(*pkt._payload) == " << sizeof(*pkt._payload) << endl;
	//cout << sizeof("teste")<<"\nteste\n";*/
	
	Communication_server com(4001);
	return 0;
}
/*struct helper_args{
	void* obj;
	int* arg;
};
class MyThread
{
public:
    MyThread() {}

    void start(helper_args* args)
    {
        args->obj = this;
        cout << endl << "start:\nobj = " << args->obj << "\narg = "<< *args->arg <<endl;
        pthread_create(&m_tid, NULL, helper, args);
    }

    void join()
    {
        pthread_join(m_tid, 0);
    }

private:
    static void* helper(void* void_args)
    {
        helper_args* args = (helper_args*)void_args;
        cout << endl << "helper:\nobj = ";
        cout << args->obj;
        cout << "\narg = ";
        cout << *args->arg <<endl;
        ((MyThread*)(args->obj))->doSomething(*args->arg);
        return 0;
    }

    void doSomething(int arg)
    {
        cout << endl << "do:\narg = "<< arg <<endl;
        std::cout << "MyThread is running!" << std::endl;
    }

    pthread_t m_tid;
};

int main()
{
    MyThread mt;
    struct helper_args args_;
    int valor = 42;
    args_.arg = &valor;
    mt.start(&args_);
    mt.join();
}*/
