#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>     /* for signal() */

#define MAXPENDING 1    /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleTCPClient(int clntSocket, char *db);   /* TCP client handling function */

#include "mylist.h"
#include "mdb.h"

#define KeyMax 5

void DieWithError(char *errorMessage)
{
        perror(errorMessage);
	    exit(1);
}

int main(int argc, char *argv[])
{
    //ignore SIGPIPE so that we don't terminate when we call
    //send() on a disconnected socket
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	DieWithError("signal() failed");

    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */

    if (argc != 3)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "%s\n", "usage: mdb-lookup-server <database_file> <port>");
        exit(1);
    }

    echoServPort = atoi(argv[2]);  /* First arg:  local port */

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        /* clntSock is connected to a client! */

        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

        HandleTCPClient(clntSock, argv[1]);
    }
    /* NOT REACHED */
}

void HandleTCPClient(int clntSocket, char *db) {
    //wrap socket file descriptor
    FILE *input = fdopen(clntSocket, "r");

    /*
     * open the database file specified in the command line
     */   

    FILE *fp = fopen(db, "rb");
    if (fp == NULL) 
        DieWithError(db);

    /*
     * read all records into memory
     */

    struct List list;
    initList(&list);

    int loaded = loadmdb(fp, &list);
    if (loaded < 0)
        DieWithError("loadmdb");
    
    fclose(fp);

    /*
     * lookup loop
     */

    char line[1000];
    char key[KeyMax + 1];

    while (fgets(line, sizeof(line), input) != NULL) {

        // must null-terminate the string manually after strncpy().
        strncpy(key, line, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        // if newline is there, remove it.
        size_t last = strlen(key) - 1;
        if (key[last] == '\n')
            key[last] = '\0';

        // traverse the list, printing out the matching records
        struct Node *node = list.head;
        int recNo = 1;
        while (node) {
            struct MdbRec *rec = (struct MdbRec *)node->data;
            if (strstr(rec->name, key) || strstr(rec->msg, key)) {
		char temp[1000];
	       	int len = sprintf(temp, "%4d: {%s} said {%s}\n", recNo, rec->name, rec->msg);
		send(clntSocket, (void *)temp, len, 0);
            }
            node = node->next;
            recNo++;
        }
    }

    // see if fgets() produced error
    if (ferror(stdin)) 
        DieWithError("stdin");

    /*
     * clean up and quit
     */

    freemdb(&list);
    fclose(input);
}
