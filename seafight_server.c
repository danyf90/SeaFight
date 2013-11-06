#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "seafight_lib.c"

int main(int argc, char* argv[]){
	int i, listener, fdmax;
	char *msg, *req, ip[16];
	struct player *players = NULL, *src, *dest;
	socklen_t addrlen;
	struct sockaddr_in addr;
	fd_set masterreadset, masterwriteset, readset, writeset;
	int len;
	FD_ZERO(&masterreadset);
	FD_ZERO(&masterwriteset);
	if(argc != 3){
		printf("Comando: seafight_server <host> <porta>\n");
		exit (1);
	}
	system("clear");
	printf("SEAFIGHT SERVER\n\n");
	//creazione del socket server
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("errore nella creazione del socket listener");
		exit(EXIT_FAILURE);
	}
	if(strcmp(argv[1], "localhost") == 0)
		strcpy(ip, "127.0.0.1");
	else
		strcpy(ip, argv[1]);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
	if(bind(listener, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
		perror("errore nel collegamento del socket listener");
		exit(EXIT_FAILURE);
	}
	if(listen(listener, 10) == -1){
		perror("errore nell'apertura del socket listener");
		exit(EXIT_FAILURE);
	}
	printf("Server in ascolto: %s:%s\n\n", argv[1], argv[2]);
	FD_SET(listener, &masterreadset);
	fdmax = listener;
	
	while(1){
		readset = masterreadset;
		writeset = masterwriteset;
		if(select(fdmax+1, &readset, &writeset, NULL, NULL) == -1){
			perror("errore nella select");
			exit(EXIT_FAILURE);
		}
		for(i = 0; i <= fdmax; i++){
			if(FD_ISSET(i, &readset)){ //il socket e' pronto in lettura
				if(i == listener){ //nuovo giocatore
					//alloco la struttura dati e la inserisco nella lista (nome = NULL)
					struct player *new = malloc(sizeof(struct player));
					if((new->socket = accept(listener, (struct sockaddr*)&addr, &addrlen)) == -1){
						perror("errore nell'accettazione di un nuovo giocatore");
						free(new);
					}
					else{
						FD_SET(new->socket, &masterreadset);
						if(new->socket > fdmax)
							fdmax = new->socket;
						new->address = addr;
						new->opponent = NULL;
						new->name = NULL;
						new->state = BUSY;
						addPlayer(&players, new);
						inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, 16);
						printf("Connessione stabilita con il client %s\n", ip);
					}
				}
				else{ //messaggio da un giocatore
					src = getBySocket(players, i); //ricavo il mittente dal file descriptor
					len = recvObject(i, (void**)&msg); //leggo la richiesta
					if(len == 0){ //il giocatore ha chiuso il socket
						inet_ntop(AF_INET, &src->address.sin_addr.s_addr, ip, 16);
						if(src->name != NULL)
							printf("%s (%s) ha chiuso il socket inaspettatamente.\n", src->name, ip);
						else
							printf("%s ha chiuso il socket inaspettatamente.\n", ip);
						//se sto giocando invio quit all'avversario
						if(src->state == BUSY && src->name != NULL){
							msg = malloc(2);
							msg[0] = QUIT;
							msg[1] = '\0';
							sendMessage(src->opponent, &masterwriteset, msg, 2);
							printf("%s e' libero.\n", src->opponent->name);
							src->opponent->state = IDLE;
							src->opponent->opponent = NULL;
						}
						rmPlayer(&players, src);
						FD_CLR(i, &masterreadset);
						FD_CLR(i, &masterwriteset);
						FD_CLR(i, &writeset);
					}
					else{ //messaggio da un giocatore
						switch(msg[0]){
							case SETNAME://SETNAME<porta><nome>
								//invio di porta e nome
								sendMessage(src, &masterwriteset, msg, len);
								break;
							case CONNECT://CONNECT<destinatario>
								//richiesta connessione
								if(src->state == IDLE){
									if((dest = getByName(players, msg + 1)) == NULL){
										//il destinatario non esiste
										msg[0] = UNKNOWN;
										sendMessage(src, &masterwriteset, msg, len);
									}
									else if(dest->state == IDLE){
										printf("Richiesta di connessione da %s", src->name);
										printf(" per %s.\n", msg + 1);
										//il destinatario è libero
										src->state = BUSY;
										src->opponent = dest;
										dest->state = BUSY;
										dest->opponent = src;
										free(msg);
										len = 2 + strlen(src->name);
										msg = malloc(len);
										msg[0] = CONNECT;
										strcpy(msg + 1, src->name);
										sendMessage(dest, &masterwriteset, msg, len);
									}
									else{
										//il destinatario è occupato
										msg[0] = BUSY;
										sendMessage(src, &masterwriteset, msg, len);
									}
								}
								break;
							case DISCONNECT://DISCONNECT
								//disconnessione del sorgente dalla partita in corso
								if(src->state != IDLE){
									//distinguo da una disconnect o una risposta a una disconnect
									if(src->opponent != NULL){
										printf("%s si e' disconnesso", src->name);
										printf(" da %s.\n", src->opponent->name);
										src->opponent->state = WFD;
										src->opponent->opponent = NULL;
										src->opponent = NULL;
									}
									src->state = IDLE;
								}
								printf("%s e' libero.\n", src->name);
								break;
							case QUIT://QUIT
								//disconnessione del sorgente dal server
								printf("%s si e' disconnesso dal server\n", src->name);
								FD_CLR(i, &masterreadset);
								FD_CLR(i, &masterwriteset);
								FD_CLR(i, &writeset);
								rmPlayer(&players, src);
								break;
							case WHO://WHO
								//richiesta della lista dei giocatori
								sendMessage(src, &masterwriteset, msg, len);
								break;
							case REFUSED://REFUSED<nome_rifiutato>
								//il mittente ha rifiutato la connessione con <nome>
								if((dest = getByName(players, msg+1)) != NULL && dest->state == BUSY && dest->opponent == src){
									//il destinatario è ancora connesso e non ha fatto disconnect
									len = strlen(src->name) + 2;
									free(msg); //dealloco il messaggio ricevuto
									msg = malloc(len);
									msg[0] = DENIED;
									strcpy(msg + 1, src->name);
									sendMessage(dest, &masterwriteset, msg, len);
								}
								src->state = IDLE;
								src->opponent = NULL;
								printf("%s e' libero.\n", src->name);
								break;
							case ACCEPTED://ACCEPTED<nome>
								//il mittente ha accettato la connessione con <nome>
								if((dest = getByName(players, msg+1)) != NULL && dest->state == BUSY && dest->opponent == src){
									//il destinatario sta attendendo una risposta
									//costruisco ACCEPTED<sockaddr_in_mittente><mittente>
									free(msg);
									len = strlen(src->name) + 2 + sizeof(struct sockaddr_in);
									msg = malloc(len);
									msg[0] = ACCEPTED;
									*(struct sockaddr_in*)(msg + 1) = src->address;
									strcpy(msg + 1 + sizeof(struct sockaddr_in), src->name);
									sendMessage(dest, &masterwriteset, msg, len);
								}
								else{
									//l'avversario non sta attendendo nulla dal mittente
									if(src->state == WFD)
										printf("%s e' libero.\n", src->name);
									src->state = IDLE;
									src->opponent = NULL;
									free(msg);
									msg = malloc(2);
									msg[0] = QUIT;
									msg[1] = '\0';
									sendMessage(src, &masterwriteset, msg, 2);
								}
								break;
						}//switch
						free(msg);
					}//else il giocatore non ha chiuso il socket
				}//else messaggio da un giocatore
			}
			if(FD_ISSET(i, &writeset)){ //il socket e' pronto in scrittura
				struct message *mex;
				dest = getBySocket(players, i); //ricavo il mittente dal file descriptor
				mex = dest->messages;
				dest->messages = mex->next;
				if(dest->messages == NULL)
					dest->last_message = NULL;
				if(mex != NULL){
					switch(mex->msg[0]){
						case SETNAME://SETNAME<porta><nome>
							req = mex->msg + 1 + sizeof(unsigned short); //puntatore al nome
							if(checkName(players, req) == 0){ //il nome e' disponibile
								msg = malloc(2);
								msg[0] = NAMEOK;
								msg[1] = '\0';
								if(sendObject(dest->socket, msg, 2, MSG_DONTWAIT) == -1){
									printf("Errore nel contattare l'utente %s", dest->name);
									perror("");
									FD_CLR(i, &masterreadset);
									FD_CLR(i, &masterwriteset);
									rmPlayer(&players, dest);
								}
								else{
									free(msg);
									printf("%s si e' connesso.\n", req);
									dest->address.sin_port = htons(*(unsigned short*)(mex->msg + 1));
									dest->name = malloc(len - 1 - sizeof(unsigned short));
									strcpy(dest->name, req);
									dest->state = IDLE;
								}
							}
							else{
								free(mex->msg);
								mex->msg = malloc(2);
								mex->msg[0] = NAMENOK;
								mex->msg[1] = '\0';
								if(sendObject(dest->socket, mex->msg, 2, MSG_DONTWAIT) == -1){
									printf("Errore nel contattare l'utente %s", dest->name);
									perror("");
									FD_CLR(i, &masterreadset);
									FD_CLR(i, &masterwriteset);
									rmPlayer(&players, dest);
								}
							}
							break;
						case WHO: //WHO
							free(mex->msg);
							len = getList(players, &mex->msg);
							if(sendObject(dest->socket, mex->msg, len, MSG_DONTWAIT) == -1){
								printf("Errore nel contattare l'utente %s", dest->name);
								perror("");
								if(dest->state == BUSY){
									free(mex->msg);
									mex->msg = malloc(2);
									mex->msg[0] = QUIT;
									mex->msg[0] = '\0';
									sendMessage(dest->opponent, &masterwriteset, mex->msg, 2);
								}
								FD_CLR(i, &masterreadset);
								FD_CLR(i, &masterwriteset);
								rmPlayer(&players, dest);
							}
							break;
						case QUIT: //QUIT
							dest->state = IDLE;
							dest->opponent = NULL;
							if(sendObject(dest->socket, mex->msg, mex->len, MSG_DONTWAIT) == -1){
								printf("Errore nel contattare l'utente %s", dest->name);
								perror("");
								FD_CLR(i, &masterreadset);
								FD_CLR(i, &masterwriteset);
								rmPlayer(&players, dest);
							}
							break;
						case CONNECT://CONNECT<richiedente_connessione>
							if((src = getByName(players, mex->msg + 1)) == NULL || src->state == IDLE || src->opponent != dest){
								//il richiedente non è più in attesa del destinatario
								if(dest->state == BUSY && dest->opponent == src){
									//serve nel caso !connect e poi !disconnect prima che la connect 
									//sia consegnata, il dest resterebbe occupato!
									dest->state = IDLE;
									dest->opponent = NULL;
								}
							}
							else{
								free(mex->msg);
								//costruisco REQUEST<porta_mittente><nome_mittente>
								len = 2 + sizeof(struct sockaddr_in) + strlen(src->name);
								mex->msg = malloc(len);
								mex->msg[0] = REQUEST;
								*(struct sockaddr_in*)(mex->msg + 1) = src->address;
								strcpy(mex->msg + 1 + sizeof(struct sockaddr_in), src->name);
								//se non riesco a consegnare la richiesta informo il mittente
								if(sendObject(dest->socket, mex->msg, len, MSG_DONTWAIT) == -1){
									free(mex->msg);
									mex->msg = malloc(2);
									mex->msg[0] = QUIT;
									mex->msg[1] = '\0';
									sendMessage(src, &masterwriteset, mex->msg, 2);
									src->state = IDLE;
									src->opponent = NULL;
									FD_CLR(i, &masterreadset);
									FD_CLR(i, &masterwriteset);
									rmPlayer(&players, dest);
								}
							}
							break;
						case ACCEPTED://ACCEPTED<sockaddr_in_mittente><accettatore>
							if((src=getByName(players,mex->msg+1+sizeof(struct sockaddr_in)))!=NULL){
								//controllo che non abbia chiuso il programma
								if(dest->state == BUSY && dest->opponent == src){
									//controllo che non abbia fatto disconnect
									if(sendObject(dest->socket, mex->msg, mex->len, MSG_DONTWAIT)==-1){
										//se l'invio di accept fallisce lo comunico al mittente
										free(mex->msg);
										mex->msg = malloc(2);
										mex->msg[0] = QUIT;
										mex->msg[1] = '\0';
										sendMessage(src, &masterwriteset, mex->msg, 2);
										src->state = IDLE;
										src->opponent = NULL;
										FD_CLR(i, &masterreadset);
										FD_CLR(i, &masterwriteset);
										rmPlayer(&players, dest);
									}
									else
										printf("%s si e' connesso a %s.\n", dest->name, src->name);
								}
								else{
									//l'avversario non e' più in attesa di una mia risposta
									free(mex->msg);
									mex->msg = malloc(2);
									mex->msg[0] = QUIT;
									mex->msg[1] = '\0';
									src->state = IDLE;
									src->opponent = NULL;
									sendMessage(src, &masterwriteset, mex->msg, 2);
								}
							}
							break;
						case DENIED: //DENIED<nome_rifiutante>
							if((src = getByName(players, mex->msg + 1)) != NULL && dest->state == BUSY && dest->opponent == src){
								//sta aspettando una mia risposta
								dest->opponent = NULL;
								dest->state = IDLE;
								if(sendObject(dest->socket, mex->msg, mex->len, MSG_DONTWAIT) == -1){
									FD_CLR(i, &masterreadset);
									FD_CLR(i, &masterwriteset);
									rmPlayer(&players, dest);
								}
							}
							break;
						default:
							if(sendObject(dest->socket, mex->msg, mex->len, MSG_DONTWAIT) == -1){
								FD_CLR(i, &masterreadset);
								FD_CLR(i, &masterwriteset);
								rmPlayer(&players, dest);
							}
					}
					free(mex->msg);
					free(mex);
					if(dest->messages == NULL) //non ho altri messaggi da inviare!
						FD_CLR(i, &masterwriteset);
				}
			}//FD_ISSET in lettura
		}//for sui FD
	}//while 1
	return 0;
}
