#ifndef FILE_SERVER_H
#define FILE_SERVER_H
#include "stdint.h"
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>

using namespace std;

class File_server
{
    public:
        File_server(string path, time_t mtime);

        void start_reading();
        void done_reading();
        void start_writing();
        void done_writing();

        string get_path();
        pthread_mutex_t *get_mutex();
        time_t get_mtime();
        void set_mtime(time_t mtime);

    private:
        string path;            // path to the file
        pthread_mutex_t mutex;  // the mutex for the file
        int reading;            // number of clients currently reading the file
        bool writing;           // true if any client is reading the file, false otherwise
        bool wants_to_write;    // tru if any thread wants to edit this file
        time_t mtime;           // the modification time of the file
};

#endif // FILE_SERVER_H
