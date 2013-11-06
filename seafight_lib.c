#define WFD -3
#define WFM -2
#define INMATCH 	-1
#define IDLE 		0
#define MYTURN 		1
#define HISTURN 	2
#define TIMER 		(struct timeval){60,0}
#define SIZE 		10
#define NSHIPS 		10
#define DIMSHIPS 	31
#define WATER 		' '
#define SHIP 		'#'
#define HITS 		'X'
#define SUNK 		'^'
#define HITW 		'~'
#define CHAT 		'M'
#define HELP 		'h'
#define WHO 		'W'
#define CONNECT 	'C'
#define DISCONNECT 	'D'
#define QUIT 		'Q'
#define ENEMYMAP	'E'
#define MYMAP		'Y'
#define HIT 		'H'
#define SETNAME 	's'
#define SETMAP 		'm'
#define ERR 		'e'
#define DENIED 		'd'
#define UNKNOWN 	'U'
#define ACCEPTED 	'A'
#define BUSY 		'B'
#define REFUSED 	'R'
#define REPLY 		'r'
#define REQUEST 	'q'
#define NAMEOK 		'N'
#define NAMENOK		'Z'
#define STARTMATCH 	'S'
#define WIN 		'w'

typedef unsigned short sizetype;

struct message{
	char *msg;
	sizetype len;
	struct message *next;
};

/*
	Contiene i dati necessari alla gestione della lista e dei giocatori
		- int socket, socket TCP utilizzato per le comunicazioni client-server
		- char* nome, puntatore al nome scelto dal giocatore
		- struct sockaddr_in address, struttura contenente i dati utilizzati nelle connessioni udp
		- char state, indica lo stato del giocatore
		- struct message* messages, puntatore al primo messaggio da inviare
		- struct message* last_message, puntatore all'ultimo messaggio da inviare
		- struct player* opponent, puntatore al giocatore con cui sta giocando (NULL se è libero)
		- struct player* next, puntatore al prossimo giocatore nella lista
*/

struct player{
	int socket;
	char *name;
	struct sockaddr_in address;
	char state;
	struct message *messages;
	struct message *last_message;
	struct player *opponent;
	struct player *next;
};

/*
	Aggiunge un giocatore in testa alla lista dei giocatori
		Parametri:
			- player **p, puntatore al puntatore contenente il primo giocatore della lista
			- player *item, puntatore alla struttura rappresentante il giocatore da aggiungere
*/

void addPlayer(struct player **p, struct player* item){
	item->next = *p;
	*p = item;
}

/*
	Controlla se il nome del giocatore esiste già tra quelli nella lista
		Parametri:
			- player *p, puntatore alla testa della lista
			- char *name, nome da controllare
		Valore di Ritorno:
			- int, -1 se il nome utente è già in uso, 0 altrimenti
*/

int checkName(struct player *p, char* name){
	for(; p != NULL; p = p->next)
		if(p->name != NULL && strcmp(p->name, name) == 0)
			return -1;
	return 0;
}

/*
	Rimuove un giocatore dalla lista dei giocatori
		Parametri:
			- struct player **p, puntatore al puntatore contenente il primo giocatore della lista
			- struct player *pun, giocatore da togliere dalla lista
		Valore di ritorno:
			- int, -1 se il giocatore non esiste, 0 se l'eliminazione e' andata a buon fine
*/

int rmPlayer(struct player **p, struct player* pun){
	struct player *tmp;
	if(pun == NULL)
		return -1;
	if(*p == pun)
		*p = (*p)->next;
	else{
		for(tmp = *p; tmp != NULL && tmp->next != pun; tmp = tmp->next);
		if(tmp == NULL)
			return -1;
		tmp->next = tmp->next->next;
	}
	while(pun->messages != NULL){
		struct message *mex = pun->messages;
		pun->messages = mex->next;
		free(mex->msg);
		free(mex);
	}
	free(pun->name);
	free(pun);
	return 0;
}

/*
	Trova un giocatore a partire dal suo socket
		Parametri:
			- struct player* p, testa della lista in cui cercare il giocatore
			- int sock, file descriptor del giocatore da cercare
		Valore di Ritorno:
			- struct player*, puntatore al giocatore con socket sock, NULL se non esiste
*/

struct player* getBySocket(struct player *p, int sock){
	for(; p != NULL; p = p->next)
		if(p->socket == sock)
			return p;
	return p;
}

/*
	Trova un giocatore a partire dal suo nome
		Parametri:
			- struct player* p, testa della lista in cui cercare il giocatore
			- char *nome, nome del giocatore da cercare
		Valore di Ritorno:
			- struct player*, puntatore al giocatore di nome name, NULL se non esiste
*/

struct player* getByName(struct player *p, char *nome){
	for(; p != NULL; p = p->next)
		if(strcmp(p->name, nome) == 0)
			return p;
	return NULL;
}

/*
	Costruisce una stringa contenente l'elenco dei giocatori separato da '\n' che inizia con WHO
	 e termina con '\0'
		Parametri:
			- player *p, lista dei giocatori
			- char ** str, puntatore al puntatore che puntera' alla stringa creata
		Valore di ritorno:
			- int, numero dei caratteri che compongono la lista
*/

int getList(struct player *p, char **str){
	struct player *tmp;
	char *cur;
	int len = 2;
	for(tmp = p;tmp != NULL; tmp = tmp->next){
		if(tmp->name != NULL){
			if(tmp->state != IDLE)
				len += strlen(tmp->name) + 12; //aggiungo (occupato)
			else
				len += strlen(tmp->name) + 1;
		}
	}
	*str = malloc(len);
	**str = WHO; //il primo carattere e' WHO
	cur = *str + 1;
	for(tmp = p; tmp != NULL; tmp = tmp->next){
		if(tmp->name != NULL){
			strcpy(cur, tmp->name);
			cur += strlen(tmp->name) + 1;
			if(tmp->state != IDLE){
				strcpy(cur - 1, " (occupato)");
				cur += 11;
			}
			cur[-1] = '\n'; //sostituisco \0 con \n
		}
	}
	*cur = '\0'; //l'ultimo carattere e' '\0'
	return len;
}

/*
Elimina tutti i caratteri dallo standard input finché non trova \n (pulisco dopo una scanf)
*/

void flush(){
	while(getchar()!='\n');
}

/*
	Legge da stdin finché non trova \n, quindi lo sostituisce con \0
		Parametri:
			- char **buf, indirizzo del puntatore in cui verrà salvato l'indirizzo della stringa letta
		Ritorno:
			- int, numero di caratteri letti (incluso \0)
*/

int readLine(char **buf){
	char *p = NULL;
	unsigned int len = 0, block = 32;
	*buf = NULL;
	//leggo blocchi di caratteri
	for(; ; block <<= 2){
		//alloco la memoria per la lettura del nuovo blocco (dim. corrente + dim. blocco))
		*buf = realloc(*buf, len + block);
		//leggo un blocco a partire dal byte successivo al'ultimo letto
		fgets(&((*buf)[len]), block, stdin);
		//cerco \n, se c'è lo sostituisco con \0 e ho finito
		if((p = strchr(&((*buf)[len]), '\n')) != NULL){
			*p = '\0';
			break;
		}
		//se non ho letto \n aggiorno la dimensione (-1 perché la fgets legge n-1 caratteri + \0)
		len += block - 1;
	}
	return strlen(*buf) + 1;
}

/*
	Interpreta gli input da tastiera e genera il messaggio da inviare al server
		Parametri
			- char **cmd, puntatore al puntatore al comando digitato (deve essere stato allocato con
				la malloc) che poi punterà al messaggio da inviare al server
		Valore di ritorno:
			- char, identificatore del comando ricevuto (ERR se il comando non esiste)
*/

char parseCommand(char **cmd){
	char *msg;
	if(**cmd == '!'){
		if(strncmp(*cmd + 1, "hit ", 4) == 0){
			if((((*cmd)[5] >= 'a' && (*cmd)[5] <= 'j') || ((*cmd)[5] >= 'A' && (*cmd)[5] <= 'J')) && (*cmd)[6] >= '0' && (*cmd)[6] <= '9' && (*cmd)[7] == '\0'){
				//se e' '!hit ln' è un attacco, porto le coordinate in indici di matrice
				if((*cmd)[5] >= 'a')
					(*cmd)[5] -= 'a';
				else
					(*cmd)[5] -= 'A';
				(*cmd)[6] -= '0';
				msg = malloc(4);
				msg[0] = HIT;
				msg[1] = (*cmd)[5];
				msg[2] = (*cmd)[6];
				msg[3] = '\0';
				free(*cmd);
				*cmd = msg;
				return HIT;
			}
		}
		else if(strcmp((*cmd) + 1, "show_enemy_map") == 0)
			return ENEMYMAP;
		else if(strcmp((*cmd) + 1, "show_my_map") == 0)
			return MYMAP;
		else if(strcmp((*cmd) + 1, "set_map") == 0)
			return SETMAP;
		else if(strcmp((*cmd) + 1, "help") == 0)
			return HELP;
		else if(strcmp((*cmd) + 1, "who") == 0){ //devo inviare 'w\0'
			free(*cmd);
			*cmd = malloc(2);
			(*cmd)[0] = WHO;
			(*cmd)[1] = '\0';
			return WHO;
		}
		else if(strncmp((*cmd) + 1, "connect ", 8) == 0){
			//se inizia con '!connect ' è una richiesta di connessione
			char *msg = malloc(strlen(*cmd + 8));
			msg[0] = CONNECT;
			strcpy(msg + 1, *cmd + 9); //devo inviare 'c<nome_client>\0'
			free(*cmd);
			*cmd = msg;
			return CONNECT;
		}
		else if(strcmp((*cmd) + 1, "disconnect") == 0){ //devo inviare 'd\0'
			free(*cmd);
			*cmd = malloc(2);
			(*cmd)[0] = DISCONNECT;
			(*cmd)[1] = '\0';
			return DISCONNECT;
		}
		else if(strcmp((*cmd) + 1, "quit") == 0){ //devo inviare 'q\0'
			free(*cmd);
			*cmd = malloc(2);
			(*cmd)[0] = QUIT;
			(*cmd)[1] = '\0';
			return QUIT;
		}
		else if(strncmp((*cmd) + 1, "chat ", 5) == 0){
			char *msg = malloc(strlen(*cmd + 5));
			msg[0] = CHAT;
			strcpy(msg + 1, *cmd + 6); //devo inviare 'm<testo>\0'
			free(*cmd);
			*cmd = msg;
			return CHAT;
		}
	}
	return ERR;
}

/*
	Disegna sullo standard output una mappa di dimensione SIZE*SIZE
		Parametri
			char map[SIZE][SIZE], matrice in cui è salvata la mappa
*/

void printMap(char map[SIZE][SIZE]){
	int i, j, k;
	printf("\n    ");
	for (i = 0; i < SIZE; i += 1)
		printf(" %d  ", i);
	printf("\n  //");
	for (i = 0; i < SIZE; i += 1)
		printf("====");
	printf("\b\\\\\n");
	for(i = 0; i < SIZE; i++){
		printf("%c ||", 'A' + i);
		for(j = 0; j < SIZE; j++)
			if(map[i][j] >= 0 && map[i][j] < NSHIPS)
				printf(" %c |", map[i][j] + '0');
			else
				printf(" %c |", map[i][j]);
		if(i != SIZE - 1){
			printf("|\n  ||");
			for(k = 0; k < SIZE; k++)
				printf("---+");
			printf("\b||\n");
		}
		else{
			printf("|\n  \\\\");
			for (i = 0; i < SIZE; i += 1)
				printf("====");
			printf("\b//\n");
		}
	}
	printf("\n");
}

/*
	Ottiene una mappa SIZE*SIZE attraverso lo standard input
		Parametri
			char map[SIZE][SIZE], matrice in cui salvare la mappa
*/

void getMap(char map[SIZE][SIZE]){
	char x = 0, d = -1, ok =  0, ships[NSHIPS];
	int i, j, y;
	ships[0]=6;ships[1]=4;ships[2]=4;ships[3]=3;ships[4]=3;
	ships[5]=3;ships[6]=2;ships[7]=2;ships[8]=2;ships[9]=2;
	for(j = 0; j < NSHIPS; j++)
		do{
			printMap(map);
			printf("Inserisci le coordinate della nave %d di dimensione %hhd: ", j + 1, ships[j]);
			do{
				scanf(" %c%d", &x, &y);
				flush();
			}
			while(x < 'A'|| (x > 'J' && x < 'a') || x > 'j' || y < 0 || y > 9);
			if(x >= 'a')
				x -= 'a';
			else
				x -= 'A';
			printf("Inserisci l'orientamento della nave [o/v]: ");
			do{
				scanf(" %c", &d);
				flush();
			}
			while(d != 'o' && d != 'O' && d !='v' && d !='V');
			if(d == 'v' || d =='V')
				d = 'v';
			else
				d = 'o';
			if( d == 'o' && y + ships[j] > 10){
				printf("La nave esce dalla mappa, scegliere una collocazione valida\n");
				ok = 0;
			}
			else if( d == 'v' && x + ships[j] > 10){
				printf("La nave esce dalla mappa, scegliere una collocazione valida\n");
				ok = 0;
			}
			else{ //la nave è interna alla mappa
				ok = 1;
				if(d == 'o'){
					for(i = 0; i < ships[j]; i++)
						if(map[(int)x][y+i] != WATER){
							ok = 0;
							printf("La nave si sovrappone con un'altra in %c%d", x + 'A', y + i);
							printf(", scegliere una collocazione valida\n");
							break;
						}
				}
				else{
					for(i = 0; i < ships[j]; i++)
						if(map[x+i][y] != WATER){
							ok = 0;
							printf("La nave si sovrappone con un'altra in %c%d", x + i + 'A', y);
							printf(", scegliere una collocazione valida\n");
							break;
						}
				}
				if(ok == 1){
					if(d == 'o'){
						for(i = 0; i < ships[j]; i++)
							map[(int)x][y+i] = j;
					}
					else{
						for(i = 0; i < ships[j]; i++)
							map[x+i][y] = j;
					}
				}
			}
		}
		while(!ok);
	printMap(map);
}

/*
	Invia un oggetto a un socket antecedendogli la sua lunghezza
		Parametri:
			- int dest, file descriptor del socket destinatario
			- const void *obj, oggetto da inviare
			- sizetype size, dimensione dell'oggetto da inviare
			- int flags, flag da passare alla send
		Valore di ritorno:
			- size se la stringa è stata trasmessa correttamente, -1 altrimenti
*/

int sendObject(int dest, const void* obj, sizetype size, int flags){
	void* msg = malloc(size + sizeof(sizetype));
	*(sizetype*)msg = size;
	memcpy(msg + sizeof(sizetype), obj, size);
	if(send(dest, msg, size + sizeof(sizetype), flags) != size + sizeof(sizetype)){
		free(msg);
		return -1;
	}
	return size;
}

/*
	Riceve una oggetto inviato con sendObject
		Parametri:
			- int src, file descriptor del socket sorgente
			- void** str, puntatore al puntatore ai dati ricevuti
		Valore di ritorno:
			- int, dimensione dell'oggeto ricevuto, -1 se in caso di errore, 0 se il socket è chiuso
*/

sizetype recvObject(int src, void** str){
	sizetype size;
	int ret = recv(src, &size, sizeof(sizetype), 0);
	if(ret != sizeof(sizetype)){
		if(ret == 0)
			return 0;
		else
			return -1;
	}
	*str = malloc(size);
	ret = recv(src, *str, size, 0);
	if(ret != size){
		free(*str);
		if(ret == 0)
			return 0;
		else
			return -1;
	}
	return size;
}

/*
	Invia un oggetto a un socket UDP inviandone prima la lunghezza
		Parametri:
			- int dest, file descriptor del socket destinatario
			- sockaddr_in addr, struttura contenente l'indirizzo del destinatario
			- const void *obj, oggetto da inviare
			- sizetype size, dimensione dell'oggetto da inviare
		Valore di ritorno:
			- size se la stringa è stata trasmessa correttamente, -1 altrimenti
*/

int sendObjectTo(int dest, struct sockaddr_in addr, const void* obj, sizetype size){
	if(sendto(dest, &size, sizeof(sizetype), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != sizeof(sizetype))
		return -1;
	if(sendto(dest, obj, size, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != size)
		return -1;
	return size;
}

/*
	Riceve una oggetto inviato con sendObjectTo
		Parametri:
			- int src, file descriptor del socket sorgente
			- sockaddr_in addr, struttura contenente l'indirizzo del destinatario
			- void** str, puntatore al puntatore ai dati ricevuti
		Valore di ritorno:
			- int, dimensione dell'oggeto ricevuto, -1 se in caso di errore, 0 se il socket è chiuso
*/

sizetype recvObjectFrom(int src, struct sockaddr_in *addr, void** str){
	sizetype size;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int ret = recvfrom(src, &size, sizeof(sizetype), 0, (struct sockaddr*)addr, &addrlen);
	if(ret != sizeof(sizetype)){
		if(ret == 0)
			return 0;
		else
			return -1;
	}
	*str = malloc(size);
	ret = recvfrom(src, *str, size, 0, (struct sockaddr*)addr, &addrlen);
	if(ret != size){
		free(*str);
		if(ret == 0)
			return 0;
		else
			return -1;
	}
	return size;
}

/*
	Posiziona il messaggio nella coda dei messaggi in attesa dell'utente
		Parametri:
			- struct player *p, giocatore destinatario del messaggio
			- fd_set *set, fd_set da modificare per predisporre la scrittura
			- char *m, puntatore al messaggio da inviare
			- sizetype size, dimensione del messaggio
*/

void sendMessage(struct player *p, fd_set *set, char *m, sizetype size){
	struct message *mex = malloc(sizeof(struct message));
	mex->msg = malloc(size);
	memcpy(mex->msg, m, size);
	mex->len = size;
	mex->next = NULL;
	FD_SET(p->socket, set);
	if(p->messages == NULL){
		p->messages = mex;
		p->last_message = mex;
	}
	else{
		p->last_message->next = mex;
		p->last_message = mex;
	}
}
