#ifndef COMMUNICATION_SERVER_H
#define COMMUNICATION_SERVER_H
#include "File_server.h"
#include "stdint.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <sstream>

using namespace std;

typedef	struct	packet{
	uint16_t	type;		//Tipo do pacote (p.ex. DATA | CMD)
	uint16_t	seqn;		//Número de sequência
	uint32_t	total_size;		//Número total de fragmentos
	uint16_t	length;	//Comprimento do payload
	const char*	_payload;				//Dados do pacote
}	packet;

vector<File_server> user_files;
pthread_mutex_t user_files_mutex;

struct th_args{
	void* obj = NULL;
	int* newsockfd = NULL;
	string* username = NULL;
	int* thread_finished = NULL;
	vector<File_server> *user_files = NULL;
	pthread_mutex_t *user_files_mutex = NULL;
};


class Communication_server
{
	public:
		void Init(int port, int header_size, int max_payload);

		long int receive_payload(int sockfd, struct packet *pkt, int type);// Receives the _payload of the packet from the client and returns a packet struct containing the _payload
		void receive_header(int sockfd, struct packet *header);	// Receives the header of the packet from the client and returns a packet struct containing the header
		void send_file(int sockfd, string path); // Send a file to the client
		void send_string(int sockfd, string str); // Send a string to the client
		void send_int(int sockfd, int number); // Send an integer to the client
		void send_mtime(int sockfd, time_t mtime);
		void send_payload(int sockfd, char* payload);
		void receive_file(int sockfd, string path); // Receive a file from the client

		int create_folder(string path);
		int delete_folder(string path);
		int delete_file(string path);
		long get_file_size(FILE *fp);

		static void *receive_commands_helper(void *void_args);

	protected:

	private:
		int port;
		int header_size;
		int max_payload;
		int packet_size;
		// TODO: tirar tudo isso. Usar o vetor de server_files passado por args
		//struct file{time_t mtime; string name;};
		//vector<file> watched_files;

		void *accept_connections();
		void *receive_commands(int sockfd, string username, int *thread_finished);

		//TODO: vai ter que tirar tudo isso
		//bool file_is_watched(string filename);
		//void update_watched_file(string filename, time_t mtime);
		//time_t get_mtime(string filename);
		//void remove_watched_file(string filename);

		//pthread_mutex_t *get_file_mutex(vector<File_server> *user_files, string path, pthread_mutex_t *user_files_mutex);
		void update_user_file(string path, time_t mtime);
        int start_reading_file(string path);
        void done_reading_file(string path);
        void start_writing_file(string path, time_t mtime);
        void done_writing_file(string path);
		void remove_file(string path);
		bool file_exists(string path);
		string get_files_and_mtime();
		time_t get_mtime(string filename, string username);

};

#endif // COMMUNICATION_SERVER_H
