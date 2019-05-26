#include "File_server.h"

using namespace std;

File_server::File_server(string path, time_t mtime)
{
    this->path = path;
    this->mtime = mtime;
    reading = 0;
    writing = false;
    wants_to_write = false;
}

void File_server::start_reading()
{
    // Give priority to writers. If any thread is writing or waiting to write, wait until it is done.
    while(true){
        // Lock the mutex
        pthread_mutex_lock(get_mutex());
        // If no thread is writing or wants to write, this thread can read
        // Since the while reads two variables, a mutex lock is needed
        if(!writing && !wants_to_write)
            break;
        // Unlock the mutex
        pthread_mutex_unlock(get_mutex());
    }
    // Start reading
    reading++;
    // Unlock the mutex
    pthread_mutex_unlock(get_mutex());
}

void File_server::done_reading()
{
    // reading = reading - 1. A variable on the left is read by other threads. A variable on the right
    //is a critical reference. The expression does not respect at-most-once. Mutex is needed.
    // Lock the mutex
    pthread_mutex_lock(get_mutex());
    reading --;
    // Unlock the mutex
    pthread_mutex_unlock(get_mutex());
}

void File_server::start_writing()
{
    // If a thread is reading or writing, wait until it finishes. Set wants_to_write to true to tell
    //the readers that they should wait until this writer is done.
    while(true){
        // If no thread is reading or writing, this thread can write
        // Since the while reads two variables, a mutex lock is needed
        // Lock the mutex
        pthread_mutex_lock(get_mutex());
        wants_to_write = true;
        if(!reading && !writing)
            break;
        // Unlock the mutex
        pthread_mutex_unlock(get_mutex());
    }
    // Start writing;
    writing = true;
    // Unlock the mutex
    pthread_mutex_unlock(get_mutex());
}

void File_server::done_writing()
{
    // Lock the mutex
    pthread_mutex_lock(get_mutex());
    writing = false;
    wants_to_write = false;
    // Unlock the mutex
    pthread_mutex_unlock(get_mutex());
}

string File_server::get_path() {return path;}

pthread_mutex_t *File_server::get_mutex() {return &mutex;}

time_t File_server::get_mtime() {return mtime;}

void File_server::set_mtime(time_t mtime) {this->mtime = mtime;}
