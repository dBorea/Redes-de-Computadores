all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o client
	gcc -Wall server.c common.o -o server

clean:
	rm common.o client server

rclient:
	./client 127.0.0.1 12345
rserver:
	./server v4 12345
