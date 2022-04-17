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
#define BACKLOG 10   // how many pending connections queue will hold

/* Global Variables */
char g_bKeepLooping = 1;
pthread_mutex_t     g_BigLock;

struct ClientInfo
{
    pthread_t threadClient;
    char szIdentifier[100];
    int  socketClient;
};

typedef struct Player {
    char *name;
    char *client;
    char *number;
    char *score;
    char *correct;
    char *receipt_time;
} Player;

typedef struct Wordle
{
    struct Player player_info[4];
    char *word;
    int num_players;
} Wordle;

char *chat_message;

Wordle wordle;

#define BACKLOG 10   // how many pending connections queue will hold
void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_data(struct ClientInfo *pClient, char *response) {
    printf("sending: %s\n", response);
    if (send(pClient->socketClient, response, strlen(response), 0) == -1)
    {
        perror("send");     
    }
}

char *receive_data(struct ClientInfo *pClient) {
    int numBytes;
    char szBuffer[BUFFER_MAX];
    if ((numBytes = recv(pClient->socketClient, szBuffer, BUFFER_MAX-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    szBuffer[numBytes] = '\0';

    char *return_val = malloc(sizeof(szBuffer));
    memcpy(return_val, szBuffer, numBytes);
    printf("received: %s\n", return_val);
    return return_val;
}

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


void join(char *name, char *client, struct ClientInfo *pClient) {
    char *fields[2] = {name, ""};
    wordle.num_players += 1;
    if(wordle.num_players >= 4) {
        fields[1] = "No";
    }
    else {
        fields[1] = "Yes";
        Player player;
        player.name = name;
        player.client = client;
        wordle.player_info[wordle.num_players] = player;
    }

    char *contents[2] = {"Name", "Result"};
    char *response = cJSON_Print(get_message("JoinResult", contents, fields, 2, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    send_data(pClient, response);
}

void chat(char *name, char *text, struct ClientInfo *pClient) {
}

void joinInstance(char *name, char *nonce, struct ClientInfo *pClient) {

}

void guess(char *name, char *guess) {

}

void interpret_message(cJSON *message, struct ClientInfo *pClient) {

    char * message_type = cJSON_GetStringValue(cJSON_GetObjectItem(message, "MessageType"));

    if(!strcmp(message_type, "Join")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *client = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Client"));
        join(name, client, pClient);  

    }
    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Text"));
        chat(name, text, pClient);
    }

    else if(!strcmp(message_type, "JoinInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Nonce"));
        joinInstance(name, nonce, pClient);
    }

    else if(!strcmp(message_type, "Guess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *guess = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Guess"));
        //guess(name, guess);
    }
    else {
        //TODO ERROR
    }
}

void * Thread_Client (void * pData)
{
    
    struct ClientInfo * pClient;
    struct ClientInfo   threadClient;
    
    char *szBuffer;
    
    // Typecast to what we need 
    pClient = (struct ClientInfo *) pData;
    
    // Copy it over to a local instance 
    threadClient = *pClient;
    //szBuffer = receive_data(pClient);
    //interpret_message(cJSON_Parse(szBuffer), pClient);
    //free(szBuffer);
    
    while(g_bKeepLooping)
    {
        printf("looping\n");
        if(wordle.num_players == 1) {
            printf("start the game!\n");
        }
        szBuffer = receive_data(pClient);

        // Debug / show what we got3
        //printf("Received a message of %d bytes from Client %s\n", numBytes, pClient->szIdentifier);
        //printf("   Message: %s\n", szBuffer);
        
        // Do something with it
                        
        
        // This is a pretty good time to lock a mutex
        pthread_mutex_lock(&g_BigLock);
        
        // Do something dangerous here that impacts shared information
        interpret_message(cJSON_Parse(szBuffer), pClient);
        free(szBuffer);
                
        // This is a pretty good time to unlock a mutex
        pthread_mutex_unlock(&g_BigLock);
    }
    
    return NULL;
}

void Server_Lobby (char *nLobbyPort)
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
        return;
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

        // Simple bit of code but this can be helpful to detect successful connections 
         
         
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Print out this client information 
        sprintf(theClientInfo.szIdentifier, "%s-%d", s, nClientCount);
        theClientInfo.socketClient = new_fd;
        nClientCount++;

        // From OS: Three Easy Pieces 
        //   https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf 
        pthread_create(&(theClientInfo.threadClient), NULL, Thread_Client, &theClientInfo);
        
        // Bail out when the third client connects after sleeping a bit
        if(nClientCount == 3 ||wordle.num_players == 1)
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
    wordle.num_players = -1;
    Server_Lobby("41333");

    printf("Sleeping before exiting\n");
    sleep(15);
    
    printf("And we are done\n");
    fclose(DFile);
    return 0;
}   
