#include "connected_client.h"

void Connected_client::init(string username, int sockfd, int num_connections, int port, int header_size, int max_payload)
{
	this->username = username;
	this->sockfd = sockfd;
	this->num_connections = num_connections;
	this->max_connections = 2;
	this->user_files_pointer = &user_files;
	this->user_files_mutex_pointer = &user_files_mutex;

	if (pthread_mutex_init(&user_files_mutex, NULL) != 0)
        cout << endl << "Error on mutex_init";

    // Create (or open) mtimes file
	/*mtimes_file_path = getenv("HOME");
	mtimes_file_path += "/server_sync_dir_" + username + "/" + "mtimes";
    ifstream mtimes_file(mtimes_file_path.c_str());
    bool exists = mtimes_file.good();
    cout << endl << "exists: " << exists;
    cout << endl << "num_connect: " << num_connections;
	mtimes_file.close();
    // If the file exists and no other client with this username is connected,
    //get information from the mtimes file
    if(exists && num_connections==1)
    	files_from_disk();*/

	com.Init(port, header_size, max_payload);
}

void Connected_client::init(string username, int sockfd, int num_connections, int port, int header_size, int max_payload, vector<File_server> *user_files_pointer, pthread_mutex_t *user_files_mutex_pointer)
{
	this->username = username;
	this->sockfd = sockfd;
	this->num_connections = num_connections;
	this->max_connections = 2;
	this->user_files_pointer = user_files_pointer;
	this->user_files_mutex_pointer = user_files_mutex_pointer;

	com.Init(port, header_size, max_payload);
}

string *Connected_client::get_username() {return &username;}

int *Connected_client::get_sockfd() {return &sockfd;}

int Connected_client::get_num_connections() {return num_connections;}

pthread_t Connected_client::get_thread() {return thread;}

void Connected_client::set_thread(pthread_t thread) {this->thread = thread;}

vector<File_server> *Connected_client::get_user_files()
{
	return user_files_pointer;
}

int Connected_client::get_user_files_size()
{
	int num_files = 0;
	cout << endl << "User: " << username;
	cout << endl << "Existem " << user_files.size() << " arquivos";
	for(int i=0; i<user_files.size(); i++)
	{
		cout << endl << "The file " << user_files[i].get_filename() << " has mtime == " << user_files[i].get_mtime();
		if(user_files[i].get_mtime()!=-1)
			num_files++;
	}
	return num_files;
}

pthread_mutex_t *Connected_client::get_user_files_mutex()
{
	return user_files_mutex_pointer;
}

int Connected_client::new_connection()
{
	if(num_connections+1 > max_connections)
		return -1;
	else
	{
		num_connections++;
		return num_connections;
	}
}

void Connected_client::remove_connection() {num_connections--;}

void Connected_client::files_from_disk()
{
	// The structure of the file is:
	//path_to_file1 mtime1
	//path_to_file2 mtime2
	//...
	ifstream mtimes_file(mtimes_file_path.c_str());
	string path;
	time_t mtime;
	while(mtimes_file >> path >> mtime)
	{
		cout << endl << "Reading mtimes file... ";
		cout << endl << "path: " << path << endl << "mtime: " << mtime;
		cout << endl;
		File_server new_file(path, mtime);
		user_files.push_back(new_file);

		string test = user_files[0].get_path();
		cout << endl << "test: |" << test << "|";
		cout << endl;
	}
}
