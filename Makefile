all: seafight_server seafight_client

seafight_server: seafight_server.c seafight_lib.c
	gcc -o server_seafight seafight_server.c

seafight_client: seafight_client.c seafight_lib.c
	gcc -o client_seafight seafight_client.c

clean:
	rm server_seafight
	rm client_seafight