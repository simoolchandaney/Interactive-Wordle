
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


cJSON *get_message(char *message_type, char *contents[], char *fields[]) {
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "MessageType", message_type);
    cJSON *data = cJSON_CreateArray();
    
    for(int i = 0; i < sizeof(contents); i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON *field = cJSON_CreateString(fields[0]);
        cJSON_AddItemToObject(item, contents[0], field);
        cJSON_AddItemToArray(data, item);
    }

    cJSON_AddArrayToObject(message, "Data", data);

    return message;
}

void interpret_message(cJSON *message) {
    char *message_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(message, "MessageType"));
    
    if(!strcmp(message_type, "JoinResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *result = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO JOINRESULT
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
    else if(!strcmp(message_type, "StartInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *server = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *port = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *nonce = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
			//TODO STARTINSTANCE
        }
        else {
            //TODO ERROR
        }
    }
    else if(!strcmp(message_type, "JoinInstanceResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *number = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *result = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
		    //TODO JOININSTANCERESULT
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "StartGame")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *rounds = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char names[array_size];
			char numbers[array_size];

			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				names[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name"));
				numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number"));
				i++;
			}

		    //TODO JOININSTANCERESULT
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "StartRound")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 4) {
            char *word_length = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *round = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *rounds_remaining = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
			int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 3));
			char names[array_size];
			char numbers[array_size];
			char scores[array_size];
			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 3)) {
				names[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name"));
				numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number"));
				scores[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Score"));
				i++;
			}
		    //TODO STARTROUND
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "PromptForGuess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *word_length = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *guess_number = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
		    //TODO PROMPTFORGUESS
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "GuessResponse")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *guess = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *accepted = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
		    //TODO GUESSRESPONSE
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "GuessResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char names[array_size];
			char numbers[array_size];
			char corrects[array_size];
			char receipt_times[array_size];
			char results[array_size];
			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				names[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name"));
				numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number"));
				corrects[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Correct"));
				receipt_times[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "ReceiptTime"));
				results[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Result"));
				i++;
			}
		    //TODO GUESSRESULT
        }
        else {
            //TODO ERROR
        }
    
    }

	else if(!strcmp(message_type, "EndRound")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *rounds_remaining = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char names[array_size];
			char numbers[array_size];
			char scores_earned[array_size];
			char winners[array_size];

			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				names[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name"));
				numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number"));
				scores_earned[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "ScoreEarned"));
				winners[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Winner"));
				i++;
			}
		    //TODO ENDROUND
        }
        else {
            //TODO ERROR
        }
    
    }
	//TODO
	else if(!strcmp(message_type, "EndGame")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *winner_name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char names[array_size];
			char numbers[array_size];
			char scores[array_size];

			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				names[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name"));
				numbers[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number"));
				scores[i] = cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Score"));
				i++;
			}
		    //TODO ENDGAME
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


int main(int argc, char *argv[]) 
{

    char player_name[BUFSIZ];
    char server_name[BUFSIZ];
    char lobby_port[BUFSIZ];
    char game_port[BUFSIZ];
    char nonce[BUFSIZ];

    for(int i = 1; i < argc; i++) {

        if(!strcmp(argv[i], "-name")) {
            strcpy(player_name, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-server")) {
            strcpy(server_name, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-port")) {
            strcpy(lobby_port, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-game")) {
            strcpy(lobby_port, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-nonce")) {
            strcpy(nonce, argv[i+1]);
        }
        else continue;
    }

	pthread_mutex_init(&g_BigLock, NULL);
	

	printf("Sleeping before exiting\n");
	sleep(15);
	
	printf("And we are done\n");
	return 0;
}	


