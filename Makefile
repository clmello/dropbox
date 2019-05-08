CC = g++
FLAGS = -g -Wall -pthread -std=c++11
CFLAGS = $(FLAGS) $(INCLUDE)

UTILS = Comunicacao.o \

CLIENT_O = client.o

SERVER_O = server.o

.PHONY: all clean

all: dropboxserver dropboxclient

dropboxserver: $(UTILS) $(SERVER_O)
	$(CC) $(CFLAGS) -o $@ $^

dropboxclient: $(UTILS) $(CLIENT_O)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
