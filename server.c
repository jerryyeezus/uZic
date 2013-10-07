#include <stdio.h>	  /* for printf() and fprintf() */
#include <sys/socket.h>	  /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>	  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>	  /* supports all sorts of functionality */
#include <unistd.h>	  /* for close() */
#include <dirent.h>
#include <string.h>	  /* support any string ops */
#include <pthread.h>	  /* support for POSIX threads */
#include "musicProtocol.h"	/* declarations of necessary functions and data types */

#define RCVBUFSIZE 512		/* The receive buffer size */
#define SNDBUFSIZE 2048		/* The send buffer size */
#define FILEBUFSIZE 512		/* The file buffer size */
#define MAXPENDING 5

#define SERVER_DIR "./repo/"

/* Structure of arguments to pass to client thread */
struct ThreadArgs {
    int clientSock;	//Socket descriptor for client
};

/**
* get_files
*
* @param  dir FILE pointer
* @param  buffer Pre-allocated buffer that will be modified
* @return buffer gets filenames
*/
void get_files ( DIR *dir, struct dirent *ent, char *buffer )
{
    if ( ( dir = opendir ( "./repo" ) ) != NULL )
    {
        while ( ( ent = readdir ( dir ) ) != NULL )
        {

            char *d_name = ent->d_name;
            if ( *d_name != '.' && strcmp ( d_name, ".." ) != 0 )
            {
                strcat ( buffer, d_name );
                strcat ( buffer, "\n" );
            }
        }

        closedir ( dir );
    }

    strcat ( buffer, "\0" );
}

/* The main function */
int main ( int argc, char *argv[] )
{
    int serverSock;				/* Server Socket */
    int clientSock;				/* Client Socket */
    struct sockaddr_in changeServAddr;		/* Local address */
    struct sockaddr_in changeClntAddr;		/* Client address */
    unsigned short changeServPort;		/* Server port */
    unsigned int clntLen;			/* Length of address data struct */
    /*DIR *dir;
    struct dirent *ent;
    char filepath[FILENAME_MAX];
    char filename[FILENAME_MAX];
    char rcvBuf[RCVBUFSIZE];
    char sndBuf[SNDBUFSIZE];
    char fileBuf[FILEBUFSIZE];
    char serverFiles[4096];
    char *curFile;
    MusicInfo rcvInfo;
    memset ( &rcvInfo, 0, sizeof ( rcvInfo ) );
    MusicInfo sndInfo;
    memset ( &sndInfo, 0, sizeof ( sndInfo ) );*/

    /* Create new TCP Socket for incoming requests*/
    if ( ( serverSock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        exit ( 1 );
    }

    int on = 1;
    setsockopt ( serverSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof ( on ) );
    memset ( &changeServAddr, 0, sizeof ( changeServAddr ) );
    changeServAddr.sin_family = AF_INET;
    changeServAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
    changeServAddr.sin_port = htons ( 12003 );

    /* Bind to local address structure */
    if ( bind ( serverSock, ( struct sockaddr * ) &changeServAddr, sizeof ( changeServAddr ) ) < 0 )
    {
        perror ( "bind() failed." );
        exit ( 1 );
    }

    /* Listen for incoming connections */
    if ( listen ( serverSock, MAXPENDING ) < 0 )
    {
        perror ( "listen() failed." );
        exit ( 1 );
    }

    /* Loop server forever*/
    while ( 1 )
    {
        /* Accept incoming connection */
        clientSock = accept ( serverSock, ( struct sockaddr * ) &changeClntAddr, &clntLen );
        if ( clientSock < 0 )
        {
            perror ( "accept() failed." );
            exit ( 1 );
        }

	/* Create separate memory for client argument (for threads) */
	struct ThreadArgs *threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
	if(threadArgs == NULL) {
	    perror("malloc() failed");
	    exit(1);
	}
	threadArgs->clientSock = clientSock;

	/* Create client thread */ 
	pthread_t threadID;
	int returnValue = pthread_create(&threadID, NULL, ThreadMain, threadArgs);
	if(returnValue != 0) {
	    perror("pthread_create() failed");
	    exit(1);
	}
	printf("Client connected on socket %d (thread %lu)\n", clientSock, (unsigned long int) threadID); 
    }

    /*free ( rcvBuf );
    free ( sndBuf );*/

}

/**
* ThreadMain
*
* @param threadArgs Void pointer to arguments of thread
**/

void *ThreadMain(void *threadArgs) {
    /* Deallocate thread resources on return */
    pthread_detach(pthread_self());

    /* Extract socket file descriptor from argument */
    int clientSock = ((struct ThreadArgs *) threadArgs)->clientSock;

    /* Deallocate memory for argument */
    free(threadArgs);

    /* Call function to handle all requests from client on a thread */
    HandleClientRequest(clientSock);

    return (NULL);
}


void HandleClientRequest(int clientSock) {

    while(1) {
	    DIR *dir;
	    struct dirent *ent;
	    char rcvBuf[RCVBUFSIZE];
	    char sndBuf[SNDBUFSIZE];
	    char fileBuf[FILEBUFSIZE];
	    char *curFile;
	    MusicInfo rcvInfo;
	    memset ( &rcvInfo, 0, sizeof ( rcvInfo ) );
	    MusicInfo sndInfo;
	    memset ( &sndInfo, 0, sizeof ( sndInfo ) );

	    memset ( &sndInfo, 0, sizeof ( sndInfo ) );
	    memset ( rcvBuf, 0, RCVBUFSIZE );
	    memset ( sndBuf, 0, SNDBUFSIZE );
	    memset ( fileBuf, 0, FILEBUFSIZE );
	    recv ( clientSock, rcvBuf, RCVBUFSIZE, 0 );

	    /* Case list */

	    if ( Decode ( rcvBuf, RCVBUFSIZE, &rcvInfo ) )
		printf ( "Request Type: %s\n", rcvInfo.requestType );
	    if(strcmp(rcvInfo.requestType, LIST) == 0 || strcmp(rcvInfo.requestType, DIFF) == 0 || strcmp(rcvInfo.requestType, PULL) == 0)
	    	printf("Handling %s request for client on socket %d\n", rcvInfo.requestType, clientSock);

	    if ( strcmp ( rcvInfo.requestType, "list" ) == 0 )
	    {
		if ( ( dir= opendir ( "./repo" ) ) != NULL )
		{
		    while ( ( ent = readdir ( dir ) ) != NULL )
		    {
			char *d_name = ent->d_name;
			if ( *d_name != '.' && strcmp ( d_name, ".." ) != 0 )
			{
			    strcat ( sndInfo.songNames, d_name );
		            strcat ( sndInfo.songNames, "|" );
		        }
		    }
		    closedir ( dir );
		}
		strcpy ( sndInfo.requestType, rcvInfo.requestType );
		strcpy ( sndInfo.songIDs, " " );
		strcpy ( sndInfo.fileData, " " );
		sndInfo.eof = 1;
		sndInfo.terminate = 1;
		sndInfo.dataLen = 0;
		size_t responseSize = Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
		send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
	    }	// end of list

	    else if ( strcmp ( rcvInfo.requestType, "diff" ) == 0 )
	    {
		if ( ( dir = opendir ( "./repo" ) ) != NULL )
		{
		    while ( ( ent = readdir ( dir ) ) != NULL )
		    {
		        char *d_name = ent->d_name;
		        if ( *d_name != '.' && strcmp ( d_name, ".." ) != 0 )
		        {
		            strcat ( sndInfo.songNames, d_name );
		            strcat ( sndInfo.songNames, "|" );
		        }
		    }
		    closedir ( dir );
		}
		strcpy ( sndInfo.requestType, rcvInfo.requestType );
		strcpy ( sndInfo.songIDs, " " );
		strcpy ( sndInfo.fileData, " " );
		sndInfo.eof = 1;
		sndInfo.terminate = 1;
		sndInfo.dataLen = 0;
		size_t responseSize = Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
		send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
	    }	// end of diff

	    /* Case pull */
	    else if ( strcmp ( rcvInfo.requestType, "pull" ) == 0 )
	    {
		memset ( &sndInfo, 0, sizeof ( sndInfo ) );
		strcpy ( sndInfo.requestType, rcvInfo.requestType );
		strcpy ( sndInfo.songIDs, rcvInfo.songIDs );

		curFile = strtok ( rcvInfo.songIDs, "|" );
		while ( curFile )
		{
		    char curDir[FILENAME_MAX];
		    strcpy ( curDir, SERVER_DIR );
		    strcat ( curDir, curFile );
		    sndInfo.eof = 0;
		    sndInfo.terminate = 0;

		    FILE *fp = fopen ( curDir, "rb" );
		    size_t bytesRead = 0;
		    while ( ( bytesRead = fread ( sndInfo.fileData, 1, 512, fp ) ) > 0 )
		    {
			if ( bytesRead < 512 )
			{
			    sndInfo.eof = 1;
			    curFile = strtok ( NULL, "|" );
			    if ( curFile == NULL )
				sndInfo.terminate = 1;
			}

			sndInfo.dataLen = bytesRead;

			if ( curFile == NULL )
			    strcpy ( sndInfo.songNames, " " );
			else
			    strcpy ( sndInfo.songNames, curFile );

		        /* Encode and send file chunk */
		        Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
			send ( clientSock, sndBuf, SNDBUFSIZE, 0 );

			memset ( sndInfo.fileData, 0, 512 );
		    }	// end of current file

		            fclose ( fp );
		}

		/* No more files at this point */
		printf ( "All files done sending.\n" );
		/*
		sndInfo.terminate = 1;
		Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
		send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
		*/

	    }		// end of pull
	    else if ( strcmp ( rcvInfo.requestType, "leave" ) == 0 )
	    {		
		printf("Closing connection with client on socket %i\n", clientSock);
		strcpy ( sndInfo.requestType, rcvInfo.requestType );
		strcpy ( sndInfo.songNames, " " );
		strcpy ( sndInfo.songIDs, " " );
		sndInfo.eof = 1;
		sndInfo.terminate = 1;
		sndInfo.dataLen = 0;
		strcpy ( sndInfo.fileData, " " );
		size_t responseSize = Encode ( &sndInfo, sndBuf, SNDBUFSIZE );
		send ( clientSock, sndBuf, SNDBUFSIZE, 0 );
		close(clientSock);
		break;
		//free(rcvBuf);
		//free(sndBuf);
		//free(sndInfo);
	    }
    }
}


