all: seafight_server seafight_client

seafight_server: seafight_server.c seafight_lib.c
	gcc -o server_seafight seafight_server.c
	
seafight_client: seafight_client.c seafight_lib.c
	gcc -o client_seafight seafight_client.c
	
Wall: Wall_seafight_server Wall_seafight_client

Wall_seafight_server: seafight_server.c seafight_lib.c
	gcc -Wall -o server_seafight seafight_server.c
	
Wall_seafight_client: seafight_client.c seafight_lib.c
	gcc -Wall -o client_seafight seafight_client.c
