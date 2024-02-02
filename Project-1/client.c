/*
 * Auth: Alan Ramirez
 * Date: 11-18-23  (Due: 12-3-23)
 * Course: CSCI-3550 (Sec: 002)
 * Desc: PROJECT-01, client().
 */

/* #include necessary libraries for this project */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* define/create necessary global variables/functions */
#define BUFFER_SIZE (100*1024*1024)
int inFile;
int inBytes;
int outBytes;
int fCounter; 
int sockfd;
char *fBuffer = NULL;
void cleanup(void);
void SIGINT_handler(int sig);
struct in_addr ia;
struct sockaddr_in sa;
unsigned short int port;

void cleanup(void) {
    /* cleanup: free buffer, close socket, close current file */
    if (fBuffer != NULL) {
        free(fBuffer);
        fBuffer = NULL;
    }
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    if (inFile >= 0) {
        close(inFile);
        inFile = -1;
    }
    return;
}

void SIGINT_handler (int sig) {
    /* signal handler, listening for ctrl+c; calls cleanup() then closes program*/
    fprintf(stderr, "client: Client interrupted. Shutting down.\n");
    cleanup();
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) { 
    /* main program ONLY runs with files, checked by this if statement */
    if (argc > 3) { 
        /* signal declared at the top; waiting for ctrl+c */
        signal(SIGINT, SIGINT_handler);

        /* firstly, ensure port number is not privileged, then create a socket
        address structure using that port, IP from argv, and IPv4 addresses */
        port = (unsigned short int) atoi (argv[2]);
        if (0 <= port && port <= 1023) {
            fprintf(stderr, "client: ERROR: Port number is privileged.\n");
            cleanup();
            exit(EXIT_FAILURE);
        }
        ia.s_addr = inet_addr(argv[1]);
        sa.sin_port = htons(port);
        sa.sin_family = AF_INET;
        sa.sin_addr = ia;

        /* next, for each file, the program establishes a new connection with the server, 
        allocates memory to a buffer for the file, opens the file, writes the file to 
        the buffer, sends the file to the server, and call cleanup(). */
        for (fCounter = 3; fCounter < argc; ++fCounter) { 
            /* here, establish the connection by creating a socket structure, 
            then connecting to the server using the socket */
            printf("client: Connecting to %s:%s...\n", argv[1], argv[2]);
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                fprintf(stderr, "client: ERROR: Failed to create socket.\n");
                cleanup();
                exit(EXIT_FAILURE);
            }
            if (connect(sockfd, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
                fprintf(stderr, "client: ERROR: Connecting to %s:%s.\n", argv[1], argv[2]);
                cleanup();
                exit(EXIT_FAILURE);
            }
            
            /* here, allocate memory for the file, open the file, read the file,
            send the file to the server, then print a success message before
            calling cleanup() */
            printf("client: Success!\nclient: Sending: \"%s\"...\n", argv[fCounter]);
            fBuffer = (char *) malloc(BUFFER_SIZE);
            if (fBuffer == NULL) {
                fprintf(stderr, "client: ERROR: Failed to allocate memory.\n");
                cleanup();
                exit(EXIT_FAILURE);
            }
            inFile = open(argv[fCounter], O_RDONLY);
            if (inFile <= 0) {
                fprintf(stderr, "client: ERROR: Failed to open: %s.\n", argv[fCounter]);
                cleanup();
                continue;
            }
            inBytes = read(inFile, fBuffer, BUFFER_SIZE);
            if (inBytes < 0) {
                fprintf(stderr, "client: ERROR: Unable to read: %s.\n", argv[fCounter]);
                cleanup();
                exit(EXIT_FAILURE);
            }
            outBytes = send(sockfd, (const void*) fBuffer, inBytes, 0);
            if (outBytes != inBytes) {
                fprintf(stderr, "client: ERROR: While sending data.\n");
                cleanup();
                exit(EXIT_FAILURE);
            }
    
            printf("client: Done.\n");
            cleanup();
        }
        
        /* finally, after iterating through all files, send a success message, then exit */
        printf("client: File transfer(s) complete.\nclient: Closing connection.\nclient: Goodbye!\n");
        exit(EXIT_SUCCESS);
    } else { 
        /* if no files are included in the execution, print this usage guide ... */
        printf("client: USAGE: client <server_IP> <server_Port> file1 file2 ...\n"); 
        exit(EXIT_FAILURE);
    }
}