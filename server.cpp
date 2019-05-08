#include "Communication_server.h"
#include <iostream>
#include <pthread.h>
//#include <fstream>

using namespace std;

int main()
{
	cout << sizeof("teste")<<"\nteste\n";
	Communication_server com(4002);
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
