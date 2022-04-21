#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "../cJSON.h"


/* Global Defines */
#define BUFFER_MAX 1000
#define BACKLOG 10   // how many pending connections queue will hold

/* Global Variables */
pthread_mutex_t     g_BigLock;

struct time {
    int tm_sec; // seconds (from 0 to 59)
    int tm_min; // minutes (from 0 to 59)
    int tm_hour; // hours (from 0 to 23)
};

struct ClientInfo
{
    pthread_t threadClient;
    char szIdentifier[100];
    int  socketClient;
};

typedef struct Input {
    int numPlayers; //total number expected
    char *lobbyPort;
    char *playPort;
    int numRounds;
    char *fileName;
    int dbg;
} Input;

typedef struct Game_Player {
    char *name;
    char *client;
    int number;
    char *guess;
    char *score;
    char *correct;
    char *receipt_time;
    char *result;
    char *score_earned;
    char *winner;
} Game_Player;

typedef struct Lobby_Player {
    char *name;
    char *client;
} Lobby_Player;

typedef struct Wordle
{
    struct Lobby_Player lobbyPlayers[10];
    struct Game_Player players[10];
    char *word;
    int num_in_lobby; //current num in lobby
    int num_players; //current num in game
    int nonce;
    char *winner_name;
    char *winner;
    int created_game;
    int game_over;
    char *chat_message;
    int num_guessed;
    struct Input inputs;
} Wordle;

Wordle wordle;
bool dbg;

#define BACKLOG 10   // how many pending connections queue will hold
char *interpret_message(cJSON *message, struct ClientInfo threadClient);
cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length);
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

char *get_IP() {
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;

  
    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));
  
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
  
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));

    return IPbuffer;
}


void send_data(struct ClientInfo threadClient, char *response) {
    if(dbg) {
        printf("sending to %d: %s\n", threadClient.socketClient, response);
    }
    if (send(threadClient.socketClient, response, strlen(response), 0) == -1)
    {
        perror("send");     
    }
}

char *receive_data(struct ClientInfo threadClient) {
    int numBytes;
    char szBuffer[BUFFER_MAX];
    if ((numBytes = recv(threadClient.socketClient, szBuffer, BUFFER_MAX-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    szBuffer[numBytes] = '\0';

    char *return_val = malloc(sizeof(szBuffer));
    memcpy(return_val, szBuffer, numBytes);
    if(dbg) {
        printf("received: %s\n", return_val);
    }
    return return_val;
}

int get_nonce() {
    int r = rand() % 1000;
    wordle.nonce = r;
    return r;
}

// function that selects random word from text file
char * word_to_guess() {
    int wordCount = 0;
    FILE *file_handle = fopen ("terms.txt", "r");
    char chunk[BUFSIZ];
    strcpy(chunk, "");
    while(fgets(chunk, sizeof(chunk), file_handle) != NULL) {
        //fputs(chunk, stdout);
        wordCount++;
    }
    //printf("wordCount is: %d\n", wordCount);
    srand(time(NULL));
    int r = (rand() % wordCount) + 1;
    //printf("%d\n", r);
    rewind(file_handle);

    int i;
    char chunk2[BUFSIZ];
    for (i = 0; i < r; i++) {
       fgets(chunk2, sizeof(chunk2), file_handle); 
    }
    chunk2[strlen(chunk2)  - 1] = '\0';
    fclose(file_handle);
    char *word = chunk2;
    for(int i = 0; i < strlen(word); i++) {
        word[i] = toupper(word[i]);
    }
    return word;
}

char * word_guess_color_builder(char * guess, char * key) {
    //TODO FIX
    //char *letters = (char*) malloc(strlen(guess)*sizeof(char));
    static char letters[BUFSIZ];
    strcpy(letters, "");
    int l, i, j, k; 
    for (l = 0; l < strlen(guess); l++) {
        letters[l] = 'B'; // grey
    }
    for (i = 0; i < strlen(guess); i++) {
        for (j = 0; j < strlen(key); j++) {
            if (guess[i] == key[j]) {
                letters[i] = 'Y'; // yellow 
            }
        }
    }
    for (k = 0; k < strlen(guess); k++) {
        if (guess[k] == key[k]) {
            letters[k] = 'G'; // green 
        }
    }
    //free(letters);
    return (char *) letters;
}


int check_profanity(char *message) {
    char *profanity[] = {"fuck", "shit", "bitch", "damn", "pussy", "penis", "dick", "cock", "bastard", "asshole"};

    for(int i = 0; i < sizeof(profanity)/sizeof(profanity[0]); i++) {
        if(strstr(message, profanity[i])) {
            return 1;
        }
    }
    return 0;
}


void promptGuess(int guess_number, char *name, char *word_length_s, struct ClientInfo threadClient) {

    char *contents1[3] = {"WordLength", "Name", "GuessNumber"};
    char guess_number_s[2];
    sprintf(guess_number_s, "%d", guess_number);
    char *fields1[3]  = {"", name, ""};
    fields1[0] = word_length_s;
    fields1[2] = guess_number_s;
    char *response = cJSON_Print(get_message("PromptForGuess", contents1, fields1, 3));
    send_data(threadClient, response);
}

char *awaitGuess(char * data, struct ClientInfo threadClient) {
    char * result = interpret_message(cJSON_Parse(data), threadClient);
    free(data);
    return result;
}

cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length) {
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "MessageType", message_type);
    cJSON *data = cJSON_CreateObject();
    for(int i = 0; i < content_length; i++) {
        if(!strcmp(message_type, "StartGame") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }
        else if(!strcmp(message_type, "StartRound") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name",wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddStringToObject(item, "Score", wordle.players[j].score);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "GuessResult") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddStringToObject(item, "Correct", wordle.players[j].correct);
                cJSON_AddStringToObject(item, "ReceiptTime", wordle.players[j].receipt_time);
                cJSON_AddStringToObject(item, "Result", wordle.players[j].result);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "EndRound") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddStringToObject(item, "ScoreEarned", wordle.players[j].score_earned);
                cJSON_AddStringToObject(item, "Winner", wordle.players[j].winner);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "EndGame") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddStringToObject(item, "Score", wordle.players[j].score);
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

void join(char *name, char *client, struct ClientInfo threadClient) {
    //make sure name is unique
    int unique  = 1;
    for(int i = 0; i < wordle.num_in_lobby; i++) {
        if (!strcmp(name, wordle.lobbyPlayers[i].name)) {
            unique = 0;
        }
    }
    if(!strcmp(name, "mpwordle")) {
        unique = 0;
    }

    char *fields[2] = {name, ""};
    if(unique == 0) {
        fields[1] = "No";
    }
    else {
        fields[1] = "Yes";
        Lobby_Player player;
        player.name = name;
        player.client = client;
        wordle.lobbyPlayers[wordle.num_in_lobby] = player;
    }
    char *contents[2] = {"Name", "Result"};
    char *response = cJSON_Print(get_message("JoinResult", contents, fields, 2));
    wordle.num_in_lobby += 1;
    send_data(threadClient, response);
}

void chat(char *name, char *text, struct ClientInfo threadClient) {
    /*
    char chat_buff[BUFSIZ];
    snprintf(chat_buff, "%s: %s", name, test);
    char *message = chat_buff;
    */
}

void joinInstance(char *name, char *nonce, struct ClientInfo threadClient) {
    //check if name and nonce is valid
    int unique = 1;
    for(int i =0; i < wordle.num_players; i++) {
        if(!strcmp(name, wordle.players[i].name)) {
            unique = 0;
        }
    }
    if(!strcmp(name, "mpwordle")) {
        unique = 0;
    }

    char buffer[2];
    sprintf(buffer, "%d", wordle.num_in_lobby);
    char *fields[3] = {name, buffer, ""};
    if(!unique || atoi(nonce) != wordle.nonce) {
        fields[2] = "No";
    }
    else {
        fields[2] = "Yes";
        Game_Player player;
        player.name = name;
        player.number = wordle.num_players + 1;
        wordle.players[wordle.num_players] = player;
        wordle.num_players += 1;
    }
    char *contents[3] = {"Name", "Number", "Result"};
    char *response = cJSON_Print(get_message("JoinInstanceResult", contents, fields, 3));
    send_data(threadClient, response);
}

char *checkGuess(char *name, char *guess, struct ClientInfo threadClient) {
    char *accepted = "Yes";
    for(int i = 0; i < strlen(guess); i++) {
        guess[i] = toupper(guess[i]);
    }
    //check for invalid word
    if(strlen(wordle.word) != strlen(guess)) {
        accepted = "No";
    }

    //todo check if word is correct and update correct
    for(int i = 0; i < wordle.num_players; i++) {
        if(!strcmp(wordle.players[i].name, name)) {
            if(!strcmp(wordle.word, guess)) {
                wordle.players[i].correct = "Yes";
                wordle.winner = "Yes";
            }
            else {
                wordle.players[i].correct = "No";
            }
            wordle.players[i].guess = guess;
            time_t tmi;
            struct  time* utcTime;
            time(&tmi);
            utcTime = (struct time *) gmtime(&tmi);
            char time_buff[100];
            snprintf(time_buff, sizeof(time_buff), "%2d:%02d:%02d", (utcTime->tm_hour) % 24, utcTime->tm_min, utcTime->tm_sec);
            //TODO change below
            //printf("time: %s\n", time_buff);
            strcpy(wordle.players[i].receipt_time, time_buff);
            wordle.players[i].result = word_guess_color_builder(guess, wordle.word);
            break;
        }
    }
    

    char *contents[3] = {"Name", "Guess", "Accepted"};
    char *fields[3] = {name, guess, accepted};
    char *response = cJSON_Print(get_message("GuessResponse", contents, fields, 3));
    send_data(threadClient, response);
    return  accepted;
}

char *interpret_message(cJSON *message, struct ClientInfo threadClient) {
    char * message_type = cJSON_GetStringValue(cJSON_GetObjectItem(message, "MessageType"));
    if(!strcmp(message_type, "Join")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *client = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Client"));
        join(name, client, threadClient); 

    }
    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Text"));
        chat(name, text, threadClient);
    }

    else if(!strcmp(message_type, "JoinInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Nonce"));
        joinInstance(name, nonce, threadClient);
        return name;
    }

    else if(!strcmp(message_type, "Guess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *guess = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Guess"));
        return checkGuess(name, guess, threadClient);
    }
    else {
        //TODO ERROR
    }
    return "";
}

void * Thread_Game (void * pData)
{
    struct ClientInfo * pClient;
    struct ClientInfo   threadClient;
    
    char *szBuffer;
    
    // Typecast to what we need 
    pClient = (struct ClientInfo *) pData;
    
    // Copy it over to a local instance 
    threadClient = *pClient;

    //receive join instance
    szBuffer = receive_data(threadClient);
    pthread_mutex_lock(&g_BigLock);
    char *name = interpret_message(cJSON_Parse(szBuffer), threadClient);
    free(szBuffer);
    pthread_mutex_unlock(&g_BigLock);

    //loop until all players have joined
    while(1) {
        pthread_mutex_lock(&g_BigLock);
        if(wordle.num_players == wordle.inputs.numPlayers) {
            pthread_mutex_unlock(&g_BigLock);
            break;
        }
        else {
            pthread_mutex_unlock(&g_BigLock);
        }
    }
    //send start game message
    pthread_mutex_lock(&g_BigLock);
    wordle.winner = "No";
    char *contents[2] = {"Rounds", "PlayerInfo"};
    char buffer[2];
    sprintf(buffer, "%d", wordle.inputs.numRounds);
    char *fields[2] = {buffer, NULL};
    char *response = cJSON_Print(get_message("StartGame", contents, fields, 2));
    send_data(threadClient, response);
    pthread_mutex_unlock(&g_BigLock);

    //send start round messages
    int num_round = 1;
    pthread_mutex_lock(&g_BigLock);
    int rounds_remaining = wordle.inputs.numRounds;
    char *word = word_to_guess();
    wordle.word = word;
    printf("word is: %s\n", word);
    while(rounds_remaining > 0) {
        char *contents[4] = {"WordLength", "Round", "RoundsRemaining", "PlayerInfo"};
        char word_length_s[2];
        sprintf(word_length_s, "%d", (int)strlen(word));
        char round_s[2];
        sprintf(round_s, "%d", num_round);
        char rounds_remaining_s[2];
        sprintf(rounds_remaining_s, "%d", rounds_remaining);
        char *fields[4] = {"", "", "", NULL};
        fields[0] = word_length_s;
        fields[1] = round_s;
        fields[2] = rounds_remaining_s;
        //reset winner fields
        for(int i = 0; i < wordle.num_players; i++) {
            wordle.players[i].winner = "No";
        }
        char *response = cJSON_Print(get_message("StartRound", contents, fields, 4));
        send_data(threadClient, response);
        wordle.num_guessed = 0;
        pthread_mutex_unlock(&g_BigLock);
        sleep(2);
        rounds_remaining--;
        int guess_number = 1;

        //prompt for guess
        while(1) {
            pthread_mutex_lock(&g_BigLock);
            promptGuess(guess_number, name, word_length_s, threadClient);
            pthread_mutex_unlock(&g_BigLock);
            guess_number++;

            char *data = receive_data(threadClient);
            pthread_mutex_lock(&g_BigLock);
            if(!strcmp(awaitGuess(data, threadClient), "Yes")) {
                wordle.num_guessed++;
                pthread_mutex_unlock(&g_BigLock);
                break;
            }
            else {
                pthread_mutex_unlock(&g_BigLock);
            }
        }
        sleep(1);
        //stall until all guesses are received
        while(1) {
            pthread_mutex_lock(&g_BigLock);
            if(wordle.num_guessed == wordle.num_players) {
                pthread_mutex_unlock(&g_BigLock);
                break;
            }
            else {
                pthread_mutex_unlock(&g_BigLock);
            }
        }
        //send guess result
        pthread_mutex_lock(&g_BigLock);
        char *contents2[2] = {"Winner", "PlayerInfo"};
        char *fields2[2] = {wordle.winner, NULL};
        response = cJSON_Print(get_message("GuessResult", contents2, fields2, 2));
        send_data(threadClient, response);
        pthread_mutex_unlock(&g_BigLock);

        sleep(1);
        pthread_mutex_lock(&g_BigLock);
        if(!strcmp(wordle.winner, "Yes")) {
           strcpy(rounds_remaining_s, "0");
        }
        char *contents3[2] = {"RoundsRemaining", "PlayerInfo"};
        char rounds_remaining_s_2[2];
        sprintf(rounds_remaining_s_2, "%d", rounds_remaining);
        char *fields3[2] = {rounds_remaining_s_2, NULL};
        response = cJSON_Print(get_message("EndRound", contents3, fields3, 2));
        send_data(threadClient, response);
        pthread_mutex_unlock(&g_BigLock);

        pthread_mutex_lock(&g_BigLock);
        if(!strcmp(wordle.winner, "Yes")) {
            break;
        }
    }
    pthread_mutex_unlock(&g_BigLock);

    //send end game since done with every round
    pthread_mutex_lock(&g_BigLock);
    sleep(1);
    char *contents4[2] = {"WinnerName", "PlayerInfo"};
    //char *winner_name;
    //TODO get winner_name
    char *fields4[2] = {"Jacob", NULL};
    response = cJSON_Print(get_message("EndGame", contents4, fields4, 2));
    send_data(threadClient, response);
    wordle.game_over = 1;
    pthread_mutex_unlock(&g_BigLock);
    return NULL;
}

void * Thread_Lobby (void * pData)
{
    
    struct ClientInfo * pClient;
    struct ClientInfo   threadClient;
    
    char *szBuffer;
    
    // Typecast to what we need 
    pClient = (struct ClientInfo *) pData;

    // Copy it over to a local instance 
    threadClient = *pClient;

    //get player info
    szBuffer = receive_data(threadClient);
    pthread_mutex_lock(&g_BigLock);
    //interpret Join message and send JoinResult back
    interpret_message(cJSON_Parse(szBuffer), threadClient);
    free(szBuffer);
    pthread_mutex_unlock(&g_BigLock);
    

    //wait until all players are in lobby
    while(1) {
        pthread_mutex_lock(&g_BigLock);
        if(wordle.num_in_lobby == wordle.inputs.numPlayers) {
            pthread_mutex_unlock(&g_BigLock);
            break;
        }
        else {
            pthread_mutex_unlock(&g_BigLock);
        }
        
    }

    //send startInstance to all clients
    pthread_mutex_lock(&g_BigLock);
    char *contents[3] = {"Server", "Port", "Nonce"};
    char buffer[5];
    sprintf(buffer, "%d", wordle.nonce);
    char *IP = get_IP();
    char *fields[3] = {IP, wordle.inputs.playPort, buffer};
    cJSON *message = get_message("StartInstance", contents, fields, 3);
    sleep(1); //make sure message sends seperately
    send_data(threadClient, cJSON_Print(message));
    pthread_mutex_unlock(&g_BigLock);

    return NULL;
    
}

void Get_Clients(char *nPort, int choice)
{
    
    // Adapting this from Beej's Guide
    
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    //struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
    struct ClientInfo theClientInfo; 

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, nPort, &hints, &servinfo)) != 0) {
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

    for(int i = 0; i< wordle.inputs.numPlayers; i++) 
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
        //sprintf(theClientInfo.szIdentifier, "%s-%d", s, nClientCount);
        theClientInfo.socketClient = new_fd;

        // From OS: Three Easy Pieces 
        //   https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf 
        if(choice == 0) {
            pthread_create(&(theClientInfo.threadClient), NULL, Thread_Lobby, &theClientInfo);
        }
        else {
            pthread_create(&(theClientInfo.threadClient), NULL, Thread_Game, &theClientInfo);
        }

    }
    close(sockfd);
}

int main(int argc, char *argv[]) {   
    srand(time(NULL));
    int numPlayers = 2;
    char *lobbyPort = "41345";
    char *playPort = "41346";
    int numRounds = 3;
    char fileName[BUFSIZ] = "../terms.txt";

    for (int i = 1; i < argc; i+=2) {
        if (!strcmp(argv[i], "-np")) {
            numPlayers = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-lp")) {
            lobbyPort = argv[i+1];
        }
        else if (!strcmp(argv[i], "-pp")) {
            playPort = argv[i+1];
        }
        else if (!strcmp(argv[i], "-nr")) {
            numRounds = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-d")) {
            sprintf(fileName, "../%s", argv[i+1]);

        }  
        else if (!strcmp(argv[i], "-dbg")) {
            dbg = true;
        }
        else if (!strcmp(argv[i], "-gameonly")) {
            // TODO: act as a game instance server only awaiting clients on port X (also ignore the provided nonce)
        }
    }
    pthread_mutex_init(&g_BigLock, NULL);
    pthread_mutex_lock(&g_BigLock);
    get_nonce();
    wordle.num_in_lobby = 0;
    wordle.game_over = 0;
    wordle.created_game = 0;
    wordle.inputs.numPlayers = numPlayers;
    wordle.inputs.lobbyPort = lobbyPort;
    wordle.inputs.playPort = playPort;
    wordle.inputs.numRounds = numRounds;
    wordle.inputs.fileName = fileName;
    pthread_mutex_unlock(&g_BigLock);
    Get_Clients(lobbyPort, 0);
    Get_Clients(playPort, 1);


    while(1) {
        pthread_mutex_lock(&g_BigLock);
        if(wordle.game_over) {
            pthread_mutex_unlock(&g_BigLock);
            break;
        }
        else {
            pthread_mutex_unlock(&g_BigLock);
        }
    }
    printf("Sleeping before exiting\n");
    sleep(15);
    
    printf("Exiting\n");
    return 0;
}   