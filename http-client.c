#include <sys/socket.h> /* for socket(), bind(), connect(), and getaddrinfo() */
#include <sys/types.h>  /* for getaddrinfo() */
#include <netdb.h>      /* for getaddrinfo() */

#include <stdio.h>      /* for printf() and fprintf() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>     /* for signal() */

#define MAXPENDING 1    /* Maximum outstanding connection requests */

void die(char *errorMessage);  /* Error handling function */
int getaddrinfo(const char *node, // e.g. "www.example.com" or IP
       const char *service, // e.g. "http" or port number
       const struct addrinfo *hints,
       struct addrinfo **res);

void die(char *errorMessage)
{
        perror(errorMessage);
	    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
	fprintf(stderr, "Usage: ./http-client <host> <port_num> <file path>");
	exit(1);
    }
    
    //from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo, *rp;  // servinfo will point to the results, rp is used to go through the linked list

    memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
    				      //(fills sizeof(hints) bytes of memory starting from &hints with zeroes)
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    // get ready to connect
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	exit(1);
    }
   
    //from the getaddrinfo() man page
    int sfd; //socket descriptor
    for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
	sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);	//make stream socket
	if (sfd == -1) {
	    continue;
	}
	if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
	    break;                  /* Successfully connected */
	}
	close(sfd);
    }
    if (rp == NULL) {               /* No address succeeded */
	fprintf(stderr, "Could not connect\n");
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo);

    //GET request
    char request[1000];
    sprintf(request, "GET %s HTTP/1.0\r\nHost: %s:%s\r\n\r\n", argv[3], argv[1], argv[2]);

    if (send(sfd, request, strlen(request), 0) != strlen(request)) {
	die("sent different number of bytes than expected");
    }

    //wrapping socket descriptor so I can use fgets, freads to read from the socket
    FILE *webpage = fdopen(sfd, "r");
    
    char line[100];
    fgets(line, sizeof(line), webpage);
    char *expected = "HTTP/1.0 200 OK\r\n";
    if (strstr(line, expected) != NULL) { 
	fclose(webpage);
	printf("%s", line);
	exit(1);
    }
    while(1) {
	char header[1000];
	fgets(header, sizeof(header), webpage);
	char *firstchar = &(header[0]); 
	if (strcmp(firstchar, "\r\n") == 0) {
	    break;
	}
    }
  
    //open the file you're dumping into
    char *filename = strrchr(argv[3], '/')+1; //strrchr returns pointer to last occurrence of '/'
    FILE *outfile = fopen(filename, "w");
    if (outfile == NULL) {
	fclose(outfile);
	fclose(webpage);
	die("fopen failed");
    }
    
    //read from socket, dump into file
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), webpage)) > 0) {
       	if (fwrite(buf, 1, n, outfile) != n) {
	    fclose(outfile);
    	    fclose(webpage);
	    die("fwrite failed");
	}
    }    
    if (ferror(webpage)) {
	fclose(outfile);
	fclose(webpage);
	die("fread failed");
    }

    //close file I'm dumping into
    fclose(outfile);
    //close the file wrapping the socket
    fclose(webpage);
    exit(0);
}
