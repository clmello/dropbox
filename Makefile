CC = g++
FLAGS = -g -pthread -std=c++11
CFLAGS = $(FLAGS) $(INCLUDE)

UTILS_SERVER = 	Communication_server.o \
				connected_client.o \
				connected_backup.o \
				Synchronization_server.o \
				File_server.o \
				backup.o \

UTILS_CLIENT = Communication_client.o \

CLIENT_O = client.o

SERVER_O = server.o

.PHONY: all clean

all: dropboxserver dropboxclient

dropboxserver: $(UTILS_SERVER) $(SERVER_O)
	$(CC) $(CFLAGS) -o $@ $^

dropboxclient: $(UTILS_CLIENT) $(CLIENT_O)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o dropboxserver
