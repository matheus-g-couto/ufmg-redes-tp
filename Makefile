all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o client
	gcc -Wall server.c common.o -o server

clean:
	rm -f *.o
	rm -f *.out
	rm -f server
	rm -f client