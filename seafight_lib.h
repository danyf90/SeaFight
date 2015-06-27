#define WFD -3
#define WFM -2
#define INMATCH 	-1
#define IDLE 		0
#define MYTURN 		1
#define HISTURN 	2
#define TIMER 		(struct timeval) {60,0}
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

struct message {
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
struct player {
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
void addPlayer(struct player **p, struct player* item);

/*
	Controlla se il nome del giocatore esiste già tra quelli nella lista
		Parametri:
			- player *p, puntatore alla testa della lista
			- char *name, nome da controllare
		Valore di Ritorno:
			- int, -1 se il nome utente è già in uso, 0 altrimenti
*/
int checkName(struct player *p, char* name);

/*
	Rimuove un giocatore dalla lista dei giocatori
		Parametri:
			- struct player **p, puntatore al puntatore contenente il primo giocatore della lista
			- struct player *pun, giocatore da togliere dalla lista
		Valore di ritorno:
			- int, -1 se il giocatore non esiste, 0 se l'eliminazione e' andata a buon fine
*/
int rmPlayer(struct player **p, struct player* pun);

/*
	Trova un giocatore a partire dal suo socket
		Parametri:
			- struct player* p, testa della lista in cui cercare il giocatore
			- int sock, file descriptor del giocatore da cercare
		Valore di Ritorno:
			- struct player*, puntatore al giocatore con socket sock, NULL se non esiste
*/
struct player* getBySocket(struct player *p, int sock);

/*
	Trova un giocatore a partire dal suo nome
		Parametri:
			- struct player* p, testa della lista in cui cercare il giocatore
			- char *nome, nome del giocatore da cercare
		Valore di Ritorno:
			- struct player*, puntatore al giocatore di nome name, NULL se non esiste
*/
struct player* getByName(struct player *p, char *nome);

/*
	Costruisce una stringa contenente l'elenco dei giocatori separato da '\n' che inizia con WHO
	 e termina con '\0'
		Parametri:
			- player *p, lista dei giocatori
			- char ** str, puntatore al puntatore che puntera' alla stringa creata
		Valore di ritorno:
			- int, numero dei caratteri che compongono la lista
*/
int getList(struct player *p, char **str);

/*
Elimina tutti i caratteri dallo standard input finché non trova \n (pulisco dopo una scanf)
*/

void flush();

/*
	Legge da stdin finché non trova \n, quindi lo sostituisce con \0
		Parametri:
			- char **buf, indirizzo del puntatore in cui verrà salvato l'indirizzo della stringa letta
		Ritorno:
			- int, numero di caratteri letti (incluso \0)
*/
int readLine(char **buf);

/*
	Interpreta gli input da tastiera e genera il messaggio da inviare al server
		Parametri
			- char **cmd, puntatore al puntatore al comando digitato (deve essere stato allocato con
				la malloc) che poi punterà al messaggio da inviare al server
		Valore di ritorno:
			- char, identificatore del comando ricevuto (ERR se il comando non esiste)
*/
char parseCommand(char **cmd);

/*
	Disegna sullo standard output una mappa di dimensione SIZE*SIZE
		Parametri
			char map[SIZE][SIZE], matrice in cui è salvata la mappa
*/
void printMap(char map[SIZE][SIZE]);

/*
	Ottiene una mappa SIZE*SIZE attraverso lo standard input
		Parametri
			char map[SIZE][SIZE], matrice in cui salvare la mappa
*/
void getMap(char map[SIZE][SIZE]);

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
int sendObject(int dest, const void* obj, sizetype size, int flags);

/*
	Riceve una oggetto inviato con sendObject
		Parametri:
			- int src, file descriptor del socket sorgente
			- void** str, puntatore al puntatore ai dati ricevuti
		Valore di ritorno:
			- int, dimensione dell'oggeto ricevuto, -1 se in caso di errore, 0 se il socket è chiuso
*/
sizetype recvObject(int src, void** str);

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
int sendObjectTo(int dest, struct sockaddr_in addr, const void* obj, sizetype size);

/*
	Riceve una oggetto inviato con sendObjectTo
		Parametri:
			- int src, file descriptor del socket sorgente
			- sockaddr_in addr, struttura contenente l'indirizzo del destinatario
			- void** str, puntatore al puntatore ai dati ricevuti
		Valore di ritorno:
			- int, dimensione dell'oggeto ricevuto, -1 se in caso di errore, 0 se il socket è chiuso
*/
sizetype recvObjectFrom(int src, struct sockaddr_in *addr, void** str);

/*
	Posiziona il messaggio nella coda dei messaggi in attesa dell'utente
		Parametri:
			- struct player *p, giocatore destinatario del messaggio
			- fd_set *set, fd_set da modificare per predisporre la scrittura
			- char *m, puntatore al messaggio da inviare
			- sizetype size, dimensione del messaggio
*/
void sendMessage(struct player *p, fd_set *set, char *m, sizetype size);
