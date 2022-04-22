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
#include "../cJSON.h"

#define BUFFER_MAX 1000

typedef struct Inputs {
    char *name;
    char *server;
    char *lobbyPort;
    char *gamePort;
    int nonce;
} Inputs;

Inputs input;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_data(int sockfd, char *data) {
    //printf("sending to %d: %s\n", sockfd, data);
    if ((send(sockfd, data, strlen(data), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
}

cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length) {
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "MessageType", message_type);
    cJSON *data = cJSON_CreateObject();
    
    for(int i = 0; i < content_length; i++) {
        cJSON *field = cJSON_CreateString(fields[i]);
        cJSON_AddItemToObject(data, contents[i], field);
    }

    cJSON_AddItemToObject(message, "Data", data);

    return message;
}

char *receive_data(int sockfd) {
    char szBuffer[BUFFER_MAX];
    int numBytes;
    if ((numBytes = recv(sockfd, szBuffer, BUFFER_MAX - 1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
    szBuffer[numBytes] = '\0';

    char *return_val = malloc(sizeof(szBuffer));
    memcpy(return_val, szBuffer, numBytes);
    //printf("received: %s\n", return_val);
    return return_val;
}

void sendJoin(int sockfd) {
    printf("Attempting to join lobby...\n");
    char *contents[2] = {"Name", "Client"};
    char *fields[2] = {input.name, "ISJ-C"};
    char *join = cJSON_Print(get_message("Join", contents, fields, 2));
    //send Join message
    send_data(sockfd, join);
}

void sendJoinInstance(int sockfd) {
    char *contents[2] = {"Name", "Nonce"};
    char *fields[2] = {"", ""};
    fields[0] = input.name;
    char nonce_s[5];
    sprintf(nonce_s, "%d", input.nonce);
    fields[1] = nonce_s;
    //send join instance
    printf("Attempting to join game...\n");
    char *message = cJSON_Print(get_message("JoinInstance", contents, fields, 2));
    send_data(sockfd, message);
}

void sendGuess(int sockfd) {
    char guess[100];
    scanf("%s", guess);
    
    char *contents[2] = {"Name", "Guess"};
    char *fields[2] = {input.name, ""};
    fields[1] = guess;

    //send guess
    char *message = cJSON_Print(get_message("Guess", contents, fields, 2));
    send_data(sockfd, message);
}

int interpret_message(cJSON *message, int sockfd, int numPlayers);
int connectToLobby(char *player_name, char *server_name, char *lobby_port);

void joinResult(char *name, char *result, int sockfd) {
    if(!strcmp(result, "Yes")) {
        printf("Joined lobby successfully\n");
        printf("--------------------\n");
        //receive StartInstance
        sleep(1);
        char *data = receive_data(sockfd);
        interpret_message(cJSON_Parse(data), sockfd, 0);
        free(data);
    }
    else {
        printf("Name is invalid. Please input a new name: \n");
        char name[100];
        scanf("%s", name);
        input.name = name;
        sendJoin(sockfd);
        close(sockfd);
    }
}

void chat(char *name, char *text, int sockfd) {
    //TODO chat
    return;
}
void startInstance(char *server, char *port, char *nonce, int sockfd) {
    input.nonce = atoi(nonce);
    close(sockfd);
    sleep(2); //make sure lobby is created before client connects
    sockfd = connectToLobby(input.name, server, port);
    sendJoinInstance(sockfd);
    //receive joininstanceresult
    char *data = receive_data(sockfd);
    interpret_message(cJSON_Parse(data), sockfd, 0);
    free(data);
}

void joinInstanceResult(char *name, char *number, char *result, int sockfd) {
    if(!strcmp(result, "Yes")) {
        //receive start game
        printf("Joined game lobby successfully\n");
        printf("There are %s players in the game\n", number);
        printf("--------------------\n");
        char *data = receive_data(sockfd);
        interpret_message(cJSON_Parse(data), sockfd, atoi(number));
    }
    else {
        printf("Incorrect nonce or name already taken. Please input a unique name: \n");
        char name[100];
        char nonce_l[100];
        scanf("%s", name);
        printf("Please input the correct nonce: \n");
        scanf("%s", nonce_l);
        input.name = name;
        input.nonce = atoi(nonce_l);
        sendJoinInstance(sockfd);
        //receive joininstanceresult
        char *data = receive_data(sockfd);
        interpret_message(cJSON_Parse(data), sockfd, 0);
        free(data);
    }
}

void startGame(char *rounds, char *names[], char *numbers[], int sockfd, int numPlayers) {
    printf("Starting Game\n");
    printf("Players: \n");
    for(int i = 0; i < numPlayers; i++) {
        printf("%s. %s\n", numbers[i], names[i]);
    }
    printf("--------------------\n");
    sleep(1);
    char *data = receive_data(sockfd);
    interpret_message(cJSON_Parse(data), sockfd, numPlayers);
    free(data);
}

void startRound(char *word_length, char *round, char *rounds_remaining, char *names[], char *numbers[], char *scores[], int sockfd, int numPlayers) {

    printf("Round %s is starting\n", round);
    printf("%s round remaining\n", rounds_remaining);
    printf("Players: \n");
    for(int i = 0; i < numPlayers; i++) {
        printf("%s. %s's score %s\n", numbers[i], names[i], scores[i]);
    }
    printf("--------------------\n");
    sleep(1);

    //receive promptForGuess
    char *data = receive_data(sockfd);
    interpret_message(cJSON_Parse(data), sockfd, numPlayers);
    free(data);
}

void promptForGuess(char *word_length, char *name, char *guess_number, int sockfd, int numPlayers) {
    printf("Guess a %s letter word: \n", word_length);
    sendGuess(sockfd);
    sleep(1);
    //receive guessresponse
    char *data = receive_data(sockfd);
    interpret_message(cJSON_Parse(data), sockfd, numPlayers);

}

void guessResponse(char *name, char *guess, char *accepted, int sockfd, int numPlayers) {
    if(!strcmp(accepted, "Yes")) {
        printf("%s was an acceptable word \n", guess);
        printf("--------------------\n");
        printf("Waiting for other players to guess...\n");
        sleep(1);
        //receive guessresult
        char *data = receive_data(sockfd);
        interpret_message(cJSON_Parse(data), sockfd, numPlayers);
    }
    else {
        printf("%s is not a valid word. Please try again: \n", guess);
        sendGuess(sockfd);
        //receive guessresponse
        char *data = receive_data(sockfd);
        interpret_message(cJSON_Parse(data), sockfd, numPlayers);
    }
    printf("--------------------\n");

}

void guessResult(char *winner, char *name, char *names[], char *numbers[], char *corrects[], char *receipt_times[], char *results[], int sockfd, int numPlayers) {
    printf("All guesses are in for this round\n");

    if(!strcmp(winner, "Yes")) {
        printf("A player guessed the word correctly!\n");
    }

    for(int i = 0; i < numPlayers; i++) {
        if(!strcmp(corrects[i], "Yes")) {
            printf("%s. %s guessed correctly at %s\n", numbers[i], names[i], receipt_times[i]);
            
            printf("Result: %s\n", results[i]);
            for(int j = 0; j < strlen(results[i]); j++) {
                printf("%c|",results[i][j]);
            }
            printf("\n");
        }
        else {
            printf("%s. %s guessed incorrectly at %s\n", numbers[i], names[i], receipt_times[i]);
            printf("Result: %s\n", results[i]);

            printf(" ");
            for(int k = 0; k < strlen(results[i]); k++) {
                printf("- ");
            }
            printf("\n");

            printf("|");
            for(int j = 0; j < strlen(results[i]); j++) {
                printf("%c|",results[i][j]);
            }
            printf("\n ");

            for(int l = 0; l < strlen(results[i]); l++) {
                printf("- ");
            }
            printf("\n");
        }
    }
    printf("--------------------\n");

    //receive endround if winner
    sleep(1);
    char *data = receive_data(sockfd);
    interpret_message(cJSON_Parse(data), sockfd, numPlayers);
    
}

void endRound(char *rounds_remaining, char *names[], char *numbers[], char *scores_earned[], char *winners[], int sockfd, int numPlayers) {
    printf("This round has ended\n");
    printf("There are %s rounds remaining\n", rounds_remaining);
    for(int i = 0; i < numPlayers; i++) {
        printf("%s. %s's score this round: %s", numbers[i], names[i], scores_earned[i]);
        if(!strcmp(winners[i], "Yes")) {
            printf(" *Round Winner\n");
        }
        else {
            printf("\n");
        }
    }
    printf("--------------------\n");
    sleep(1);

    //receive either endgame or startround
    char *data = receive_data(sockfd);
    interpret_message(cJSON_Parse(data), sockfd, numPlayers);
}

void endGame(char *winner_name, char *names[], char *numbers[], char *scores[], int sockfd, int numPlayers) {
    printf("Game Over\n");
    printf("%s won!\n", winner_name);
    for(int i = 0; i < numPlayers; i++) {
        printf("%s. %s's score: %s\n", numbers[i], names[i], scores[i]);
    }
    printf("--------------------\n");
}

int interpret_message(cJSON *message, int sockfd, int numPlayers) {
    char *message_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(message, "MessageType"));
    if(!strcmp(message_type, "JoinResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *result = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Result"));
        joinResult(name, result, sockfd);

    }

    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
            char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
            char *text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Text"));
            //TODO CHAT
            chat(name, text, sockfd);
    }

    else if(!strcmp(message_type, "StartInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
            char *server = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Server"));
            char *port = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Port"));
            char *nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Nonce"));
            startInstance(server, port, nonce, sockfd);
    }

    else if(!strcmp(message_type, "JoinInstanceResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
            char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
            char *number = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Number"));
            char *result = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Result"));
            joinInstanceResult(name, number, result, sockfd);
    }

    else if(!strcmp(message_type, "StartGame")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *rounds = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Rounds"));
        //array size should not be passed

        char *names[numPlayers];
        char *numbers[numPlayers];
        cJSON * playerInfo = cJSON_GetObjectItemCaseSensitive(data, "PlayerInfo");
        cJSON *entry;
        int i = 0;
        cJSON_ArrayForEach(entry, playerInfo) {
            names[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Name"));
            numbers[i] =cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Number"));
            i++;
        }
        startGame(rounds, names, numbers, sockfd, numPlayers);
    
    }
    else if(!strcmp(message_type, "StartRound")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *word_length = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "WordLength"));
        char *round = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Round"));
        char *rounds_remaining = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "RoundsRemaining"));
        char *names[numPlayers];
        char *numbers[numPlayers];
        char *scores[numPlayers];
        cJSON *playerInfo = cJSON_GetObjectItemCaseSensitive(data, "PlayerInfo");
        cJSON *entry;
        int i = 0;
        cJSON_ArrayForEach(entry, playerInfo) {
            names[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Name"));
            numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Number"));
            scores[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Score"));
            i++;
        }
        startRound(word_length, round, rounds_remaining, names, numbers, scores, sockfd, numPlayers);
    }

    else if(!strcmp(message_type, "PromptForGuess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *word_length = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "WordLength"));
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *guess_number = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "GuessNumber"));
        promptForGuess(word_length, name, guess_number, sockfd, numPlayers);
    }

    else if(!strcmp(message_type, "GuessResponse")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *guess = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Guess"));
        char *accepted = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Accepted"));
        guessResponse(name, guess, accepted, sockfd, numPlayers);
    }

    else if(!strcmp(message_type, "GuessResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *winner = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Winner"));
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *names[numPlayers];
        char *numbers[numPlayers];
        char *corrects[numPlayers];
        char *receipt_times[numPlayers];
        char *results[numPlayers];
        cJSON *playerInfo = cJSON_GetObjectItemCaseSensitive(data, "PlayerInfo");
        cJSON *entry;
        int i = 0;
        cJSON_ArrayForEach(entry, playerInfo) {
            names[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name"));
            numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Number"));
            corrects[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Correct"));
            receipt_times[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "ReceiptTime"));
            results[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Result"));
            i++;
        }
        guessResult(winner, name, names, numbers, corrects, receipt_times, results, sockfd, numPlayers);
    }

    else if(!strcmp(message_type, "EndRound")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *rounds_remaining = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "RoundsRemaining"));
        char *names[numPlayers];
        char *numbers[numPlayers];
        char *scores_earned[numPlayers];
        char *winners[numPlayers];
        cJSON *playerInfo = cJSON_GetObjectItemCaseSensitive(data, "PlayerInfo");
        cJSON *entry;
        int i = 0;
        cJSON_ArrayForEach(entry, playerInfo) {
            names[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Name"));
            numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Number"));
            scores_earned[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "ScoreEarned"));
            winners[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Winner"));
            i++;
        }
        endRound(rounds_remaining, names, numbers, scores_earned, winners, sockfd, numPlayers);
    }

    else if(!strcmp(message_type, "EndGame")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *winner_name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "WinnerName"));
        char *names[numPlayers];
        char *numbers[numPlayers];
        char *scores[numPlayers];
        cJSON *playerInfo = cJSON_GetObjectItemCaseSensitive(data, "PlayerInfo");
        cJSON *entry;
        int i = 0;
        cJSON_ArrayForEach(entry, playerInfo) {
            names[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Name"));
            numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Number"));
            scores[i]=  cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(entry, "Score"));
            i++;
        }
        endGame(winner_name, names, numbers, scores, sockfd, numPlayers);
    }
    
    return sockfd;
}

int connectToLobby(char *player_name, char *server_name, char *lobby_port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    //struct sockaddr_in *h;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_name, lobby_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    //printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

    return sockfd;
}

int main(int argc, char *argv[]) 
{

    char player_name[BUFSIZ];
    char server_name[BUFSIZ];
    char lobby_port[BUFSIZ];


    for(int i = 1; i < argc; i++) {

        if(!strcmp(argv[i], "-name")) {
            strcpy(player_name, argv[i+1]);
            input.name = player_name;
        }
        else if(!strcmp(argv[i], "-server")) {
            strcpy(server_name, argv[i+1]);
            input.server = server_name;
        }
        else if(!strcmp(argv[i], "-port")) {
            strcpy(lobby_port, argv[i+1]);
            input.lobbyPort = lobby_port;
        }
        else continue;
    }

    //todo create thread for chat
    int sockfd = connectToLobby(player_name, server_name, lobby_port);
    

    sendJoin(sockfd);
    //receive joinResult message
    char *data = receive_data(sockfd);
    sockfd = interpret_message(cJSON_Parse(data), sockfd, 0);
    free(data);

    return 0;
}   
