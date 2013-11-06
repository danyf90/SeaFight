#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include "seafight_lib.c"

extern int errno;

int main(int argc, char* argv[]){
	char originalmap[SIZE][SIZE], mymap[SIZE][SIZE], enemymap[SIZE][SIZE], t, *msg, *req_name, ships[NSHIPS];
	char tosink, *cmd = NULL, *username = NULL, *opponent_name = NULL, turno = IDLE, fine = 0, ip[16];
	const int STDIN = fileno(stdin);
	int fdmax, server, udp, i = 1;
	sizetype len;
	unsigned short port;
	fd_set masterset,readset;
	struct sockaddr_in opponent, src;
	struct timeval timer;
	FD_ZERO(&masterset);
	memset(originalmap, WATER, SIZE * SIZE);
	if(argc != 3){
		printf("Usage: seafight_client <host remoto> <porta>\n");
		exit(EXIT_FAILURE);
	}
	system("clear");
	printf("SEAFIGHT CLIENT\n\n");
	//connessione al server
	if((server = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("errore nella creazione del socket TCP");
		exit(EXIT_FAILURE);
	}
	if(strcmp(argv[1], "localhost") == 0)
		strcpy(ip, "127.0.0.1");
	else
		strcpy(ip, argv[1]);
	memset(&opponent, 0, sizeof(struct sockaddr_in));
	opponent.sin_family = AF_INET;
	opponent.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, ip, &opponent.sin_addr.s_addr);
	if(connect(server, (struct sockaddr*)&opponent, sizeof(struct sockaddr_in)) == -1){
		perror("errore nella connessione al server");
		exit(EXIT_FAILURE);
	}
	//connessione avvenuta, richiesta ed invio di nome e porta
	printf("Connessione al server %s:%s effettuata con successo\n\n", argv[1], argv[2]);
	printf("Sono disponibili i seguenti comandi:\n * !help --> mostra l'elenco dei comandi disponibi");
	printf("li\n * !who --> mostra l'elenco dei client connessi al server\n * !connect nome_client -");
	printf("-> avvia una partita con l'utente nome_client\n * !disconnect --> disconnette il client ");
	printf("dall'attuale partita\n * !quit --> disconnette il client dal server\n * !show_enemy_map ");
	printf("--> mostra la mappa dell'avversario\n * !show_my_map --> mostra la propria mappa\n * !se");
	printf("t_map --> reimposta la mappa\n * !hit <coordinates> --> esegue un'attacco\n * !chat mess");
	printf("aggio> --> invia un messaggio di chat all'avversario\n\nInserisci il tuo nome: ");
	do
		len = readLine(&username);
	while(len == 0);
	printf("Inserisci la porta udp di ascolto: ");
	scanf("%hd", &port);
	flush();
	//creazione socket udp
	if((udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("errore nella creazione del socket udp");
		exit(EXIT_FAILURE);
	}
	memset(&opponent, 0, sizeof(struct sockaddr_in));
	opponent.sin_family = AF_INET;
	opponent.sin_port = htons(port);
	opponent.sin_addr.s_addr = htonl(INADDR_ANY);
	while(bind(udp, (struct sockaddr*) &opponent, sizeof(struct sockaddr_in)) == -1){
		switch(errno){
			case EADDRINUSE: //porta già in uso
				printf("La porta scelta è occupata, sceglierne un'altra: ");
				scanf("%hd", &port);
				flush();
				opponent.sin_port = htons(port);
				break;
			case EACCES:
				printf("Non hai i permessi per quella porta, prova una porta superiore a 1023: ");
				scanf("%hd", &port);
				flush();
				opponent.sin_port = htons(port);
				break;
			default:
				perror("errore non gestito nel bind del socket udp");
				exit(EXIT_FAILURE);
		}
	}
	getMap(originalmap);
	ships[0]=6;ships[1]=4;ships[2]=4;ships[3]=3;ships[4]=3;
	ships[5]=3;ships[6]=2;ships[7]=2;ships[8]=2;ships[9]=2;
/*	ships = (char[]){6,4,4,3,3,3,2,2,2,2};*/
	memcpy(mymap, originalmap, SIZE*SIZE);
	len += 1 + sizeof(unsigned short);
	msg = malloc(len);
	msg[0] = SETNAME;
	*(unsigned short*)(msg + 1) = port;
	strcpy(msg + 1 + sizeof(unsigned short), username);
	len = sendObject(server, msg, len, 0);
	if(len > 0)
		len = recvObject(server, (void**)&cmd);
	else{
		printf("Il server non e' piu' disponibile...\n");
		free(msg);
		free(cmd);
		free(username);
		close(server);
		return 0;
	}
	while(*cmd != NAMEOK){
		printf("Il nome e' gia' in uso, scegline un altro: ");
		do
			len = readLine(&username);
		while(len == 0);
		len += 1 + sizeof(unsigned short);
		msg = realloc(msg, len);
		strcpy(msg + 1 + sizeof(unsigned short), username);
		len = sendObject(server, msg, len, 0);
		if(len > 0)
			len = recvObject(server, (void**)&cmd);
		else{
			printf("Il server non e' piu' disponibile...\n");
			free(msg);
			free(cmd);
			free(username);
			close(server);
			return 0;
		}
	}
	free(cmd);
	free(msg);
	//preparazione dei fd_set per la select
	FD_SET(server, &masterset);
	FD_SET(STDIN, &masterset);
	if(server >= STDIN)
		fdmax = server;
	else
		fdmax = STDIN;

	while(!fine){
		if(turno == HISTURN || turno == MYTURN)
			printf("# ");
		else
			printf("> ");
		fflush(stdout);
		readset = masterset;
		if(turno == HISTURN || turno == MYTURN)
			i = select(fdmax+1, &readset, NULL, NULL, &timer);
		else
			i = select(fdmax+1, &readset, NULL, NULL, NULL);
		if(i == -1){
			perror("errore nella select");
			free(username);
			free(opponent_name);
			close(server);
			close(udp);
			exit(EXIT_FAILURE);
		}
		else if(i == 0){
			//è scaduto il timer
			if(turno == MYTURN)
				printf("\rE' passato un minuto dal tuo ultimo comando inviato... HAI PERSO!\n\n");
			else
				printf("\rL'avversario non risponde da piu' di un minuto... HAI VINTO!\n\n");
			turno = IDLE;
			memcpy(mymap, originalmap, SIZE*SIZE);
			FD_CLR(udp, &masterset);
			if(udp == fdmax){
				if(server >= STDIN)
					fdmax = server;
				else
					fdmax = STDIN;
			}
			cmd = malloc(2);
			cmd[0] = DISCONNECT;
			cmd[1] = '\0';
			if(sendObject(server, cmd, 2, 0) == -1){
				perror("!timeout_disconnect: errore nel contattare il server");
				free(cmd);
				free(username);
				free(opponent_name);
				close(server);
				close(udp);
				exit(EXIT_FAILURE);
			}
			free(cmd);
		}
		else
			for(i = 0; i <= fdmax; i++){
				if(FD_ISSET(i, &readset)){
					//ricevuto comando da tastiera
					if(i == STDIN && readLine(&cmd) != 0){
						t = parseCommand(&cmd);
						switch(t){
							case HELP:
								if(turno == IDLE){ //non sono impegnato in una partita
									printf("Sono disponibili i seguenti comandi:\n * !help --> mostr");
									printf("a l'elenco dei comandi disponibili\n * !who --> mostra l");
									printf("'elenco dei client connessi al server\n * !connect nome_");
									printf("client --> avvia una partita con l'utente nome_client\n ");
									printf("* !quit --> disconnette il client dal server\n * !set_ma");
									printf("p --> reimposta la mappa\n * !show_my_map --> mostra la ");
									printf("propria mappa\n");
								}
								else{
									printf("Sono disponibili i seguenti comandi:\n * !help --> mostr");
									printf("a l'elenco dei comandi disponibili\n * !who --> mostra l");
									printf("'elenco dei client connessi al server\n * !disconnect --");
									printf("> disconnette il client dall'attuale partita\n * !quit -");
									printf("-> disconnette il client dal server\n * !show_enemy_map ");
									printf("--> mostra la mappa dell'avversario\n * !show_my_map -->");
									printf(" mostra la propria mappa\n * !hit <coordinates> --> eseg");
									printf("ue un'attacco\n * !chat <messaggio> --> invia un messagg");
									printf("io di chat all'avversario\n");
								}
								break;
							case WHO: //invio WHO'\0'
								if(sendObject(server, cmd, 2, 0) == -1){
									perror("!who: errore nel contattare il server");
									free(cmd);
									free(username);
									free(opponent_name);
									close(server);
									close(udp);
									exit(EXIT_FAILURE);
								}
								break;
							case CONNECT: //invio CONNECT'<nome_client>\0'
								if(turno == IDLE){
									if(strcmp(username, cmd + 1) == 0)
										printf("Non puoi iniziare una partita con te stesso!\n");
									else{
										if(sendObject(server, cmd, strlen(cmd) + 1, 0) == -1){
											perror("!connect: errore nel contattare il server");
											free(cmd);
											free(username);
											free(opponent_name);
											close(server);
											close(udp);
											exit(EXIT_FAILURE);
										}
										turno = WFM;
									}
								}
								else{
									printf("Sei impegnato in una partita, se vuoi iniziarne un'altra");
									printf(" devi prima interrompere questa (!disconnect)\n");
								}
								break;
							case DISCONNECT: //invio DISCONNECT'\0'
								if(turno == MYTURN || turno == HISTURN){
									//sono in partita
									if(sendObjectTo(udp, opponent, cmd, 2) == -1)
										perror("!disconnect: errore nel contattare l'avversario.");
									if(sendObject(server, cmd, 2, 0) == -1){
										perror("!disconnect: errore nel contattare il server.");
										free(cmd);
										free(username);
										free(opponent_name);
										close(server);
										close(udp);
										exit(EXIT_FAILURE);
									}
									printf("Partita interrotta: HAI PERSO!\n\n");
									turno = IDLE;
									memcpy(mymap, originalmap, SIZE*SIZE);
								}
								else if(turno == WFM){
									//voglio annullare la richiesta di connessione
									if(sendObject(server, cmd, 2, 0) == -1){
										perror("!disconnect: errore nel contattare l'avversario.");
										free(cmd);
										free(username);
										free(opponent_name);
										close(server);
										close(udp);
										exit(EXIT_FAILURE);
									}
									printf("Richiesta annullata.\n");
									turno = IDLE;
								}
								else{
									printf("Non sei impegnato in nessuna partita, per disconnetterti");
									printf(" dal server usa !quit\n");
								}
								break;
							case QUIT: //invio QUIT'\0 oppure 'DISCONNECT'\0', DISCONNECT'\0', QUIT'\0'
								if(turno == IDLE){
									if(sendObject(server, cmd, 2, 0) == -1)
										perror("!quit: errore nel contattare il server");
								}
								else{ //se sono in una partita mando prima disconnect
									cmd[0] = DISCONNECT;
									if(sendObject(server, cmd, 2, 0) == -1)
										perror("!disconnect: errore nel contattare il server");
									if(sendObjectTo(udp, opponent, cmd, 2) == -1)
										perror("!disconnect: errore nel contattare l'avversario");
									cmd[0] = QUIT;
									if(sendObject(server, cmd, 2, 0) == -1)
										perror("!quit: errore nel contattare il server");
								}
								fine = 1;
								break;
							case ENEMYMAP:
								if(turno == HISTURN || turno == MYTURN)
									printMap(enemymap);
								else
									printf("Non sei impegnato in nessuna partita.\n");
								break;
							case MYMAP:
								printMap(mymap);
								break;
							case HIT:
								switch(turno){
									case IDLE: case WFM:
										printf("Non sei impegnato in nessuna partita.\n");
										break;
									case HISTURN:
										printf("Attendi il tuo turno...\n");
										break;
									case MYTURN:
										if(sendObjectTo(udp, opponent, cmd, 4) == -1)
											perror("!hit: errore nell'invio dell'attacco");
										break;
								}
								break;
							case CHAT:
								if(turno == IDLE || turno == WFM){
									printf("Non sei impegnato in nessuna partita, non puoi inviare");
									printf(" messaggi di chat.\n");
								}
								else
									if(sendObjectTo(udp, opponent, cmd, strlen(cmd) + 1) == -1)
										perror("errore durante l'invio di un messaggio di chat");
								break;
							case ERR:
								printf("Comando non valido, digita !help per la lista dei comandi.\n");
								break;
							case SETMAP:
								if(turno == IDLE){
									memset(originalmap, WATER, SIZE * SIZE);
									getMap(originalmap);
									memcpy(mymap, originalmap, SIZE*SIZE);
								}
								else
									printf("Non puoi cambiare la tua mappa mentre sei in partita.\n");
								break;
						}
						free(cmd);
					}//endif comandi da tastiera
					else if(i == server){ //ricevuto un messaggio dal server
						len = recvObject(i, (void**)&cmd); //richiesta da un giocatore
						if(len == 0){ //il server ha chiuso il socket
							printf("\rIl server non e' più disponibile...\n");
							free(username);
							free(opponent_name);
							close(udp);
							close(server);
							exit(EXIT_FAILURE);
						}
						switch(*cmd){
							case UNKNOWN: //nome utente non valido
								if(turno == WFM){
									printf("\rImpossibile connettersi a %s:", cmd + 1);
									printf(" utente inesistente.\n");
									turno = IDLE;
								}
								break;
							case BUSY: //utente occupato
								if(turno == WFM){
									printf("\rImpossibile connettersi a %s:", cmd +1);
									printf(" utente impegnato in un'altra partita.\n");
									turno = IDLE;
								}
								break;
							case ACCEPTED: //l'utente ha accettato la partita
								//salvo l'indirizzo e il nome
								if(turno == WFM){
									opponent = *(struct sockaddr_in*)(cmd + 1);
									free(opponent_name);
									opponent_name = malloc(strlen(cmd+1+sizeof(struct sockaddr_in))+1);
									strcpy(opponent_name, cmd + 1 + sizeof(struct sockaddr_in));
									printf("\r%s ha accettato la partita.\n", opponent_name);
									//inizializzo le variabili di partita e invio STARTMATCH
									free(cmd);
									cmd = malloc(2);
									ships[0]=6;ships[1]=4;ships[2]=4;ships[3]=3;ships[4]=3;
									ships[5]=3;ships[6]=2;ships[7]=2;ships[8]=2;ships[9]=2;
									cmd[0] = STARTMATCH;
									cmd[1] = '\0';
									sendObjectTo(udp, opponent, cmd, 2);
									printf("\rPartita avviata con %s.", opponent_name);
									printf("\n\nE' il tuo turno.\n");
									turno = MYTURN;
									tosink = NSHIPS;
									timer = TIMER;
									memset(enemymap, WATER, SIZE * SIZE);
									FD_SET(udp, &masterset);
									if(udp > fdmax)
										fdmax = udp;
								}
								break;
							case REQUEST://REQUEST<sockaddr_in><nome> l'utente ha richiesto una partita
								if(turno == IDLE){
									req_name = cmd + 1 + sizeof(struct sockaddr_in);
									printf("\rL'utente %s vuole avviare una partita con te", req_name);
									printf(", vuoi accettare? [s/n] ");
									do
										scanf(" %c", &t);
									while(t != 'S' && t != 's' && t != 'N' && t != 'n');
									flush();
									len = strlen(req_name) + 2;
									msg = malloc(len);
									strcpy(msg + 1, req_name);
									if(t == 'S' || t == 's'){ //partita accettata
										turno = WFM;
										*msg = ACCEPTED;
										if(sendObject(server, msg, len, 0) == -1){
											perror("accepted: errore nel contattare il server");
											free(msg);
											free(cmd);
											free(username);
											free(opponent_name);
											close(server);
											close(udp);
											exit(EXIT_FAILURE);
										}
										//salvo l'indirizzo e il nome
										opponent = *(struct sockaddr_in*)(cmd + 1);
										free(opponent_name);
										opponent_name = malloc(strlen(req_name) + 1);
										strcpy(opponent_name, req_name);
										//inizializzo le variabili di partita
										memset(enemymap, WATER, SIZE * SIZE);
										tosink = NSHIPS;
										FD_SET(udp, &masterset);
										if(udp > fdmax)
											fdmax = udp;
									}
									else{ //partita rifiutata
										printf("Partita rifiutata.\n");
										*msg = REFUSED;
										if(sendObject(server, msg, len, 0) == -1){
											perror("accepted: errore nel contattare il server");
											free(msg);
											free(cmd);
											free(username);
											free(opponent_name);
											close(server);
											close(udp);
											exit(EXIT_FAILURE);
										}
									}
									free(msg);
								}
								break;
							case DENIED: //l'utente ha rifiutato la partita
								if(turno == WFM){
									printf("\rImpossibile connettersi a %s:", cmd + 1);
									printf(" l'utente ha rifiutato la partita.\n");
									turno = IDLE;
								}
								break;
							case WHO: //ricevuta lista degli utenti
								printf("\rClient connessi al server:\n%s", cmd + 1);
								break;
							case QUIT:
								//stavo giocando
								printf("\r");
								if(turno == MYTURN || turno == HISTURN){
									printf("L'avversario ha chiuso il socket inaspettatamente...");
									printf("HAI VINTO\n\n");
									turno = IDLE;
									memcpy(mymap, originalmap, SIZE*SIZE);
									FD_CLR(udp, &masterset);
									if(udp == fdmax){
										if(server >= STDIN)
											fdmax = server;
										else
											fdmax = STDIN;
									}
									break;
								}
								//ero in fase di connessione
								else if(turno != IDLE){
									printf("\rIl server non riesce a contattare l'avversario\n");
									turno = IDLE;
									FD_CLR(udp, &masterset);
									if(udp == fdmax){
										if(server >= STDIN)
											fdmax = server;
										else
											fdmax = STDIN;
									}
								}
								break;
						}
						free(cmd);
					}//endif messaggio dal server
					else{ //ricevuto un messaggio dall'avversario
						len = recvObjectFrom(udp, &src, (void**)&cmd);
						if(src.sin_addr.s_addr != opponent.sin_addr.s_addr)
							printf("\rRicevuto messaggio da un giocatore che non e' l'avversario\n");
						else{
							switch(*cmd){
								case HIT:
									if(turno == HISTURN){
										printf("\rRicevuto attacco in %c%hhd: ", cmd[1] + 'A', cmd[2]);
										t = mymap[(int)cmd[1]][(int)cmd[2]];
										cmd = realloc(cmd, 5);
										*cmd = REPLY;
										cmd[4] = '\0';
										if(t != HITW && t != WATER){
											printf("colpita!\n");
											cmd[3] = mymap[(int)cmd[1]][(int)cmd[2]] = HITS;
											if(t >= 0 &&  t < NSHIPS){
												//ho preso una barca non già colpita
												if(--ships[(int)t] == 0){
													//l'ho affondata
													if(--tosink == 0)
													// se mi ha affondato tutte le navi lo comunico
														cmd[3] = WIN;
													else
														cmd[3] = SUNK;
													printf("La nave %hhd e' stata affondata.\n", t);
												}
											}
										}
										else{
											printf("acqua!\n");
											cmd[3] = mymap[(int)cmd[1]][(int)cmd[2]] = HITW;
										}
										sendObjectTo(udp, opponent, cmd, 5);
										if(tosink == 0){
											printf("La tua ultima nave e' stata affondata...");
											printf(" HAI PERSO!\n\n");
											msg = malloc(2);
											msg[0] = DISCONNECT;
											msg[1] = '\0';
											if(sendObject(server, msg, 2, 0) == -1){
												perror("!hit_lost: errore nel contattare il server");
												free(cmd);
												free(msg);
												free(username);
												free(opponent_name);
												close(server);
												close(udp);
												exit(EXIT_FAILURE);
											}
											free(msg);
											turno = IDLE;
											memcpy(mymap, originalmap, SIZE*SIZE);
										}
										else{
											turno = MYTURN;
											timer = TIMER;
											printf("\nE' il tuo turno.\n");
										}
									}
									break;
								case CHAT:
									if(turno != IDLE && turno != WFM)
										printf("\r%s: %s\n", opponent_name, cmd + 1);
									break;
								case REPLY:
									if(turno == MYTURN){
										turno = HISTURN;
										timer = TIMER;
										printf("\rAttacco in %c%hhd: ", cmd[1] + 'A', cmd[2]);
										switch(cmd[3]){
											case HITW:
												printf("acqua!\n");
												break;
											case HITS:
												printf("colpita!\n");
												break;
											case SUNK:
												printf("colpita e affondata!\n");
												break;
											case WIN:
												printf("colpita e affondata!\nHai affondato ");
												printf("l'ultima nave avversaria... HAI VINTO!\n\n");
												turno = IDLE;
												memcpy(mymap, originalmap, SIZE*SIZE);
												msg = malloc(2);
												msg[0] = DISCONNECT;
												msg[1] = '\0';
												if(sendObject(server, msg, 2, 0) == -1){
													perror("!win: errore nel contattare il server");
													free(cmd);
													free(msg);
													free(username);
													free(opponent_name);
													close(server);
													close(udp);
													exit(EXIT_FAILURE);
												}
												free(msg);
												break;
										}
										if(enemymap[(int)cmd[1]][(int)cmd[2]] == cmd[3])
											printf("Avevi gia' scelto quel bersaglio!\n");
										else if(cmd[3] == SUNK)
											enemymap[(int)cmd[1]][(int)cmd[2]] = HITS;
										else
											enemymap[(int)cmd[1]][(int)cmd[2]] = cmd[3];
										if(turno != IDLE)
											printf("\nE' il turno di %s\n", opponent_name);
									}
									break;
								case DISCONNECT:
									if(turno == MYTURN || turno == HISTURN){
										if(sendObject(server, cmd, 2, 0) == -1)
											perror("disconnect: errore nel contattare il server");
										turno = IDLE;
										memcpy(mymap, originalmap, SIZE*SIZE);
										printf("%s si e' arreso, HAI VINTO!\n\n", opponent_name);
										FD_CLR(udp, &masterset);
										if(udp == fdmax){
											if(server >= STDIN)
												fdmax = server;
											else
												fdmax = STDIN;
										}
									}
									break;
								//serve per evitare che mi dica partita avviata e poi partita conclusa
								//nel caso in cui lui faccia !connect, !disconnect
								case STARTMATCH:
									if(turno == WFM){
										printf("\rPartita avviata con %s.\n", opponent_name);
										printf("\nE' il turno di %s.\n", opponent_name);
										turno = HISTURN;
										timer = TIMER;
										memcpy(mymap, originalmap, SIZE*SIZE);
										ships[0]=6;ships[1]=4;ships[2]=4;ships[3]=3;ships[4]=3;
										ships[5]=3;ships[6]=2;ships[7]=2;ships[8]=2;ships[9]=2;
									}
									break;
							}
							free(cmd);
						}
					}//endif messaggio dall'avversario
				}
			}
	}
	free(opponent_name);
	free(username);
	close(udp);
	close(server);
	return 0;
}
