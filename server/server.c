#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "../cJSON.h"

bool debugFlag = false;


/* Global Defines */
#define BUFFER_MAX 1000

/* Global Variables */
char g_bKeepLooping = 1;
pthread_mutex_t 	g_BigLock;

struct ClientInfo 
{
	pthread_t threadClient;
	char szIdentifier[100];
	int  socketClient;
};

// fucntion that selects random word from text file
char * word_to_guess(char * file_name) {
    /*
    int c;
    int wordCount = 0;
    FILE *file_handle = fopen (file_name, "r");
    while ((c = fgetc(file_handle)) != EOF){
       wordCount++; 
    }
    int i;
    char words[wordCount];
    for (i = 0; i < wordCount; i++) {
        words[i] = malloc(11);
        fscanf (file_handle, "%s", words[i]); 
    }
    int r = rand() % wordCount;
    char * result;
    strcpy(result, words[r]);
    for (i = 0; i < wordCount; ++i) {
        free (words[i]); 
    }
    return result;
    */
}

char * word_guess_color_builder(char * guess, char * key) {
    /*char array [strlen(guess)];
    for (int l = 0; l < sizeof(array); l++) {
        strcpy(array[l], "GREY"); // grey
    }
    for (int i = 0; i < strlen(guess); i++) {
        for (int j = 0; j < strlen(key); j++) {
            if (guess[i] == key[j]) {
                strcpy(array[i], "YELLOW"); // yellow 
            }
        }
    }
    for (int k = 0; k < strlen(guess); k++) {
        if (guess[k] == key[k]) {
            strcpy(array[k], "GREEN"); // green 
        }
    }
    char * result
    for (int m = 0; m < strlen(guess); m++) {
        sprintf(result, "%c - %s ", guess[m], array[m]) 
    }
    return result;
    */
}

cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length, int num_players, char *names[], char *numbers[], char *scores[], char *corrects[], char *receipt_times[], char * results[], char *scores_earned[], char *winners[]) {
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "MessageType", message_type);
    cJSON *data = cJSON_CreateObject();

    for(int i = 0; i < content_length; i++) {
        if(!strcmp(message_type, "StartGame") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            if(! (names && numbers)) {
                //ERROR
            }
            for(int j = 0; j < num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", names[j]);
                cJSON_AddStringToObject(item, "Number", numbers[j]);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }
        else if(!strcmp(message_type, "StartRound") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            if(! (names && numbers && scores)) {
                //ERROR
            }
            for(int j = 0; j < num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", names[j]);
                cJSON_AddStringToObject(item, "Number", numbers[j]);
                cJSON_AddStringToObject(item, "Score", scores[j]);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "GuessResult") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            if(! (names && numbers && corrects && receipt_times && results)) {
                //ERROR
            }
            for(int j = 0; j < num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", names[j]);
                cJSON_AddStringToObject(item, "Number", numbers[j]);
                cJSON_AddStringToObject(item, "Correct", corrects[j]);
                cJSON_AddStringToObject(item, "ReceiptTime", receipt_times[j]);
                cJSON_AddStringToObject(item, "Result", results[j]);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "EndRound") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            if(! (names && numbers && scores_earned && winners)) {
                //ERROR
            }
            for(int j = 0; j < num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", names[j]);
                cJSON_AddStringToObject(item, "Number", numbers[j]);
                cJSON_AddStringToObject(item, "ScoreEarned", scores_earned[j]);
                cJSON_AddStringToObject(item, "Winner", winners[j]);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "EndGame") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            if(! (names && numbers && scores)) {
                //ERROR
            }
            for(int j = 0; j < num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", names[j]);
                cJSON_AddStringToObject(item, "Number", numbers[j]);
                cJSON_AddStringToObject(item, "Score", scores[j]);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else {
            cJSON *field = cJSON_CreateString(fields[i]);
            cJSON_AddItemToObject(data, contents[i], field);
        }
    }

    cJSON_AddItemToObject(message, "Data", data);

    return message;
}

void interpret_message(cJSON *message) {
    char *message_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(message, "MessageType"));
    
    if(!strcmp(message_type, "Join")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *client = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO JOIN
        }
        else {
            //TODO ERROR
        }
    }
    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *text = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO CHAT
        }
        else {
            //TODO ERROR
        }
    
    }
    else if(!strcmp(message_type, "JoinInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *nonce = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO JOININSTANCE
        }
        else {
            //TODO ERROR
        }
    }
    else if(!strcmp(message_type, "Guess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *guess = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO GUESS
        }
        else {
            //TODO ERROR
        }
    
    }
    else {
        //TODO ERROR
    }
}

void * Thread_Client (void * pData)
{
    
	struct ClientInfo * pClient;
	struct ClientInfo   threadClient;
	
	char szBuffer[BUFFER_MAX];
	int	 numBytes;
	
	/* Typecast to what we need */
	pClient = (ClientInfo *) pData;
	
	/* Copy it over to a local instance */
	threadClient = *pClient;
	
	while(g_bKeepLooping)
	{
		if ((numBytes = recv(pClient->socketClient, szBuffer, MAXDATASIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}

		szBuffer[numBytes] = '\0';

		// Debug / show what we got
		printf("Received a message of %d bytes from Client %s\n", numBytes, pClient->szIdentifier);
		printf("   Message: %s\n", szBuffer);
		
		// Do something with it
						
		
		
		// This is a pretty good time to lock a mutex
		pthread_mutex_lock(&g_BigLock);
		
		// Do something dangerous here that impacts shared information
		
		// Echo back the same message
		if (send(pClient->socketClient, szBuffer, numBytes, 0) == -1)
		{
			perror("send");		
		}
				
		// This is a pretty good time to unlock a mutex
		pthread_mutex_unlock(&g_BigLock);
	}
	
	return NULL;
}

void Server_Lobby (uint16_t nLobbyPort)
{
    
	// Adapting this from Beej's Guide
	
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
	
	struct ClientInfo theClientInfo;
	int       nClientCount = 0;	

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, nLobbyPort, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(g_bKeepLooping) 
	{  
        sin_size = sizeof their_addr;		
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		
        if (new_fd == -1) 
		{
            perror("accept");
            continue;
        }

		/* Simple bit of code but this can be helpful to detect successful
		   connections 
		 */
         
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

		// Print out this client information 
		sprintf(theClientInfo.szIdentifier, "%s-%d", s, nClientCount);
		theClientInfo.socketClient = new_fd;
		nClientCount++;

		/* From OS: Three Easy Pieces 
		 *   https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf */
		pthread_create(&(theClientInfo.threadClient), NULL, Thread_Client, &theClientInfo);
		
		// Bail out when the third client connects after sleeping a bit
		if(nClientCount == 3)
		{
			g_bKeepLooping = 0;
			sleep(15);
		}		
    }
    
}

int main(int argc, char *argv[]) 
{   
    int numPlayers = 1;
    int lobbyPort = 8900;
    int playPorts = 2;
    int numRounds = 3;
    FILE *DFile;
    DFile = fopen("../terms.txt", "r+");

    for (int i = 1; i < argc; i+=2) {
        if (!strcmp(argv[i], "-np")) {
            numPlayers = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-lp")) {
            lobbyPort = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-pp")) {
            playPorts = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-nr")) {
            numRounds = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-d")) {
            fclose(DFile);
            char fileName[BUFSIZ];
            sprintf(fileName, "../%s", argv[i+1]);
            // call function that see=lects random word 
        }  
        else if (!strcmp(argv[i], "-dbg")) {
            debugFlag = true;
        }
        else if (!strcmp(argv[i], "-gameonly")) {
            // TODO: act as a game instance server only awaiting clients on port X (also ignore the provided nonce)
        }
    }
	pthread_mutex_init(&g_BigLock, NULL);
	
	Server_Lobby(41000);

	printf("Sleeping before exiting\n");
	sleep(15);
	
	printf("And we are done\n");
    fclose(DFile);
	return 0;
}	
