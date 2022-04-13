
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
	pthread_mutex_init(&g_BigLock, NULL);
	
	Server_Lobby(41000);

	printf("Sleeping before exiting\n");
	sleep(15);
	
	printf("And we are done\n");
	return 0;
}	


