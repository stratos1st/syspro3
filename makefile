CC= g++
CGLAG=  -ggdb -Wall


aaa:client server

client: inet_str_client.o tuple.o linked_list.o
	$(CC) $(CFLAG) -o client inet_str_client.o tuple.o linked_list.o

inet_str_client.o: inet_str_client.c
	$(CC) -c inet_str_client.c

server: inet_str_server.o tuple.o linked_list.o
	$(CC) $(CFLAG) -o server inet_str_server.o tuple.o linked_list.o

inet_str_server.o: inet_str_server.c
	$(CC) -c inet_str_server.c

linked_list.o: linked_list.cpp linked_list.h
	$(CC) -c linked_list.cpp

tuple.o: tuple.cpp tuple.h
	$(CC) -c tuple.cpp

.PHONY: clean
clean:
	rm -f inet_str_server.o tuple.o linked_list.o inet_str_client.o server client
