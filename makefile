CC= g++
CFLAG=  -ggdb -Wall

all: dropbox_client dropbox_server

dropbox_client: dropbox_client.o tuple.o linked_list.o
	$(CC) $(CFLAG) -o dropbox_client dropbox_client.o tuple.o linked_list.o -lpthread

dropbox_client.o: dropbox_client.cpp
	$(CC) -c dropbox_client.cpp

dropbox_server: dropbox_server.o tuple.o linked_list.o
	$(CC) $(CFLAG) -o dropbox_server dropbox_server.o tuple.o linked_list.o -lpthread

dropbox_server.o: dropbox_server.cpp
	$(CC) -c dropbox_server.cpp

linked_list.o: linked_list.cpp linked_list.h
	$(CC) -c linked_list.cpp

tuple.o: tuple.cpp tuple.h
	$(CC) -c tuple.cpp

.PHONY: clean
clean:
	rm -f dropbox_server.o tuple.o linked_list.o dropbox_client.o dropbox_server dropbox_client
