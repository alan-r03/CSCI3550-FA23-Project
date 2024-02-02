/*
 * Auth: Alan Ramirez
 * Date: 11-18-23  (Due: 12-3-23)
 * Course: CSCI-3550 (Sec: 002)
 * Desc: PROJECT-01, server().
 */

/* #include necessary libraries for this project */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
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
int outFile;
int inBytes;
int outBytes;
int fCounter; 
int sockfd;
int val = 0;
int cl_sockfd;
char outFName[80];
char *fBuffer = NULL;
void cleanup(int x);
void SIGINT_handler(int sig);
socklen_t cl_sa_size;
struct in_addr ia;
struct sockaddr_in sa;
struct sockaddr_in cl_sa;
unsigned short int port;

void cleanup(int x) {
    /* cleanup: free buffer, close client socket, close write file; if 
    cleanup() is called with x=1, then close the server's listening 
    socket as well; this var is used in the case of errors or ctrl+c 
    calls where we would want to shut down the server completely */
    if (fBuffer != NULL) {
        free(fBuffer);
        fBuffer = NULL;
    }

    if (cl_sockfd >= 0) {
        close(cl_sockfd);
        cl_sockfd = -1;
    }

    if (outFile >= 0) {
        close(outFile);
        outFile = -1;
    }
    if (x == 1 && sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    return;
}

void SIGINT_handler (int sig) {
    /* signal handler, listening for ctrl+c; closes server socket, 
    calls cleanup(), then closes program */
    fprintf(stderr, "\nserver: Server interrupted. Shutting down.\n");
    cleanup(1);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    /* main program ONLY runs with 2 arguments, checked by this if statement 
    the two arguments must be the file, and the port number */
    if (argc == 2) { 
        /* signal declared at the top; waiting for ctrl+c */
        signal(SIGINT, SIGINT_handler);

        /* firstly, ensure port number is not privileged, then create a socket
        address structure using that port, Odin's IP, and IPv4 addresses */
        port = (unsigned short int) atoi (argv[1]);
        if (0 <= port && port <= 1023) {
            fprintf(stderr, "server: ERROR: Port number is privileged.\n");
            cleanup(1);
            exit(EXIT_FAILURE);
        }
        ia.s_addr = inet_addr("127.0.0.1");
        sa.sin_port = htons(port);
        sa.sin_family = AF_INET;
        sa.sin_addr = ia;

        /* next, create and establish the server's listening socket. first, create
        the socket, next, call setsockopt() to ensure the socket can be reused later, 
        next, bind the socket, and finally, make the socket a listening socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            fprintf(stderr, "client: ERROR: Failed to create socket.\n");
            cleanup(1);
            exit(EXIT_FAILURE);
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*) &val, sizeof(int)) != 0) {
            fprintf(stderr, "server: ERROR: Socket operation setsockopt() failed.\n");
            cleanup(1);
            exit(EXIT_FAILURE);
        }
        if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) != 0) {
            fprintf(stderr, "server: ERROR: Socket operation binding() failed.\n");
            cleanup(1);
            exit(EXIT_FAILURE);
        }
        if (listen(sockfd, 32) != 0) {
            fprintf(stderr, "server: ERROR: Socket operation listen() failed.\n");
            cleanup(1);
            exit(EXIT_FAILURE);
        }
        /* at this point, the server (if the port number is good) should be up and 
        successfully waiting for a client to send files ... */
        printf("server: Server running.\nserver: Awaiting TCP connections over port %d...\n", port);

        /* this for loop keeps count of the files names for 'file-XX.dat' and ensures that 
        the server can be accessed multiple times from different client calls */
        for (fCounter = 1; 1; ++fCounter) {
            /* first, create necessary structures and structure pointers for the client socket
            that will be given in the accept statement below */
            cl_sa_size = sizeof(cl_sa);
            memset((void*) &cl_sa, 0, sizeof(cl_sa));
            cl_sockfd = accept(sockfd, (struct sockaddr *) &cl_sa, &cl_sa_size);
            if (cl_sockfd > 0) {
                /* this branch executes when the server successfully establishes a connection with a client 
                first, allocate the memory for the incoming buffer for the recv() function call */
                printf("server: Connection accepted!\nserver: Receiving file...\n");
                fBuffer = (char *) malloc(BUFFER_SIZE);
                if (fBuffer == NULL) {
                    fprintf(stderr, "server: ERROR: Failed to allocate memory.\n");
                    cleanup(1);
                    exit(EXIT_FAILURE);
                }

                /* next, receive the file sent by the client and save it in the buffer */
                inBytes = recv(cl_sockfd, (void* ) fBuffer, BUFFER_SIZE, MSG_WAITALL);
                if (inBytes > 0) {
                    /* this branch executes when 1 or more bytes have been successfully received
                    by the server, allowing for the server to 'process()' the file. */

                    /* first, create the new dat file (using file counter) and open it for writing */
                    sprintf(outFName, "file-%02d.dat", fCounter);
                    printf("server: Saving file: \"%s\"...\n", outFName);
                    outFile = open(outFName, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
                    if (outFile < 0) {
                        fprintf(stderr, "server: ERROR: Unable to create: %s\n", outFName);
                        cleanup(1);
                        exit(EXIT_FAILURE);
                    }
                    /* next, actually write the contents of the buffer to the file */
                    outBytes = write(outFile, fBuffer, (size_t) inBytes);
                    if (outBytes != inBytes) {
                        fprintf(stderr, "server: ERROR: Unable to write: %s\n", outFName);
                        cleanup(1);
                        exit(EXIT_FAILURE);
                    }
                    printf("server: Done.\n");
                } else if (inBytes == 0) {
                    /* this branch executes if, for whatever reason, recv() returns 0, meaning the 
                    connection between server and client was closed; we just 'pass' and move along */
                } else {
                    /* this branch executes if recv() returns a value < 0; meaning there was an 
                    error reading from the socket... the server cleans up and then exits. */
                    fprintf(stderr, "server: ERROR: Reading from socket.\n");
                    cleanup(1);
                    exit(EXIT_FAILURE);
                }
            } else {
                /* this branch executes if accept() unsuccessfully establishes a connection
                with the client */
                fprintf(stderr, "server: ERROR: While attempting to accept a connection.\n");
                cleanup(1);
                exit(EXIT_FAILURE);
            }

            /* finally, after processing the current file, cleanup() without closing the server's
            listening socket and await a new client connection */
            cleanup(0);
        }
        /* safeguards placed here if the server somehow ever exits the for loop */
        cleanup(1);
        exit(EXIT_FAILURE);
    } else {
        /* this branch executes when the user enters the incorrect 
        amount of arguments upon starting the server */
        printf("server: USAGE: server <listen_Port>\n");
        exit(EXIT_FAILURE);
    }
}