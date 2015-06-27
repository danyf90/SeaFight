all: seafight_server seafight_client

seafight_server: seafight_server.o seafight_lib.o
	gcc seafight_server.o seafight_lib.o -o server_seafight

seafight_client: seafight_client.o seafight_lib.o
	gcc seafight_client.o seafight_lib.o -o client_seafight

seafight_lib.o: seafight_lib.c seafight_lib.h
	gcc -c seafight_lib.c -o seafight_lib.o

seafight_client.o: seafight_client.c
	gcc -c seafight_client.c -o seafight_client.o

seafight_server.o: seafight_server.c
	gcc -c seafight_server.c -o seafight_server.o

clean:
	rm *.o
	rm server_seafight
	rm client_seafight