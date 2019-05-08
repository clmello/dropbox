#include <iostream>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

using namespace std;

int time_between_checks = 10;
struct file{time_t mtime; string name;};
vector<file> watched_files;
string path = "/home/alack/alack_syncdir";

void check_files()
{
    int err = 0;

    // If files are already being watched
    if(watched_files.size()>0)
    {
        DIR* dir = opendir(path.c_str());
        if(dir == NULL)
            cout << "\nerror opening folder\n";

        // For each watched file
        for(unsigned int n = 0; n < watched_files.size(); n++)
        {
            // Looks for it in the folder
            bool found = false;
            for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; )
            {
                string fileName = d_struct->d_name;
                if(fileName == watched_files[n].name)
                {
                    found = true;

                    string file_ = path + "/" + d_struct->d_name;
                    struct stat fileattrib;
                    if(stat(file_.c_str(), &fileattrib) < 0)
                        cout << "\nstat error\n";

                    // Checks if file was changed
                    if(difftime(watched_files[n].mtime, fileattrib.st_mtime))
                    {
                        cout << "\n\nthe file " << d_struct->d_name << " has changed!\n It should be uploaded!\n";
                        watched_files[n].mtime = fileattrib.st_mtime;
                    }
                }
            }
            // If file isn't found, then it has been deleted
            if(!found)
            {
                cout << "\n\nthe file " << watched_files[n].name << " has been deleted!\n It should be deleted on the server!\n";
                watched_files.erase(watched_files.begin()+n);
            }
            // Reset the position of the directory stream
            rewinddir(dir);
        }

        // Checks for new files
        rewinddir(dir);
        if(dir == NULL)
            cout << "\nerror opening folder\n";
        // For every file in the folder
        for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; )
        {
            // Looks for the file in the vector
            bool found = false;
            if(d_struct->d_name[0] != '.')
            {
                for(unsigned int n = 0; n < watched_files.size(); n++)
                {
                    string fileName = d_struct->d_name;
                    if(fileName == watched_files[n].name)
                    {
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    cout << "\n\nthe file " << d_struct->d_name << " is new!\n It should be uploaded!\n";

                    // Start watching file
                    string file_ = path + "/" + d_struct->d_name;
                    struct stat fileattrib;
                    if(stat(file_.c_str(), &fileattrib) < 0)
                        cout << "\nstat error\n";

                    file newFile;
                    newFile.name = d_struct->d_name;
                    newFile.mtime = fileattrib.st_mtime;
                    watched_files.push_back(newFile);
                }
            }
        }
        closedir(dir);
    }
    // If no files are watched
    else
    {
        DIR* dir = opendir(path.c_str());
        if(dir == NULL)
            cout << "\nerror opening folder\n";

        // Loops through files in sync_dir
        for(struct dirent *d_struct = NULL; (d_struct = readdir(dir)) != NULL; )
        {
            // If there is a file
            if(d_struct->d_name[0] != '.')
            {
                // Start watching file
                string file_name = path + "/" + d_struct->d_name;
                struct stat fileattrib;
                if(stat(file_name.c_str(), &fileattrib) < 0)
                    cout << "\nstat error\n";

                file newFile;
                newFile.name = d_struct->d_name;
                newFile.mtime = fileattrib.st_mtime;
                watched_files.push_back(newFile);

                cout << "\n\nthe file " << d_struct->d_name << " is new!\n It should be uploaded!\n";
            }
        }
        closedir(dir);
    }
}

void printWatchedFiles()
{
    if(watched_files.size() > 0)
    {
        cout << "\n\nWATCHED FILES:\n";
        for(unsigned int i = 0; i < watched_files.size(); i++)
        {
            cout << watched_files[i].name << endl;
        }
        cout << endl;
    }
    else
        cout << "\nThere are no watched files\n";
}

int main()
{
    printWatchedFiles();
    check_files();
    sleep(time_between_checks);
    check_files();
    printWatchedFiles();
}
