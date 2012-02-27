/*-----------------------------------------------------------------------------
 --	SOURCE FILE:    threadServer.c - A simple thread server
 --
 --	PROGRAM:		NEED NAME
 --
 --	FUNCTIONS:		
 --                 int main(int argc, char **argv);
 --                 void server(int port);
 --                 void *processConnection(void *data);
 --                 void initializeServer(int *listenSocket, int *port);
 --                 void displayClientData(unsigned long long clients);
 --                 static void systemFatal(const char *message);
 --
 --	DATE:			February 8, 2012
 --
 --	REVISIONS:		(Date and Description)
 --
 --	DESIGNERS:      Luke Queenan
 --
 --	PROGRAMMERS:	Luke Queenan
 --
 --	NOTES:
 -- A simple thread server
 ----------------------------------------------------------------------------*/

/* System includes */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* User includes */
#include "network.h"

int main(int argc, char **argv);
void server(int port);
void *processConnection(void *data);
void initializeServer(int *listenSocket, int *port);
void displayClientData(unsigned long long clients);
static void systemFatal(const char *message);

typedef struct
{
    int socket;
    int commSocket;
} clientData;

/*
 -- FUNCTION: main
 --
 -- DATE: Feb 20, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int main(argc, char **argv)
 --
 -- RETURNS: 0 on success
 --
 -- NOTES:
 -- This is the main entry point for the thread server
 */
int main(int argc, char **argv)
{
    // Initialize port and give default option in case of no user input
    int port = DEFAULT_PORT;
    int option = 0;
    
    // Parse command line parameters using getopt
    while ((option = getopt(argc, argv, "p:")) != -1)
    {
        switch (option)
        {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -p [port]\n", argv[0]);
                return 0;
        }
    }
    
    // Start server
    server(port);
    
    return 0;
}

/*
 -- FUNCTION: server
 --
 -- DATE: Feb 20, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: void server(int)
 --
 -- RETURNS: void
 --
 -- NOTES:
 -- This function contains the server loop for the thread server. It blocks
 -- on accept, and when a connection is detected, a new thread is spawned to
 -- handle the client.
 */
void server(int port)
{
    int listenSocket = 0;
    int socket = 0;
    int comms[2];
    unsigned long long connectedClients = 0;
    char clientIp[16];
    long data = 0;
    pthread_t thread = 0;
    pthread_attr_t attr;
    
    /* Create the socket pair for sending data for collection */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, comms) == -1)
    {
        systemFatal("Unable to create socket pair");
    }
    
    /* Create the reusable thread attributes */
    pthread_attr_init(&attr);
    
    /* Set the thread to non joinable (detached) */
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        systemFatal("Unable to set thread attributes to detached");
    }
    
    /* Set the thread for kernel management. This means that system calls will
     not block all threads in the process and that individual threads can be
     assigned to indiviudal processors in the system. */
    if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) != 0)
    {
        systemFatal("Unable to set thread to system scope");
    }
    
    /* Initialize the server */
    initializeServer(&listenSocket, &port);
    
    while (1)
    {
        /* Block on accepting connections */
        if ((socket = acceptConnectionIp(&listenSocket, clientIp)) == -1)
        {
            systemFatal("Unable to accept client");
        }
        
        /* Store the data needed in the thread */
        data = (long)socket << sizeof(int) | comms[1];
        
        /* Create the thread */
        if (pthread_create(&thread, &attr, processConnection,
                           (void *) data) != 0)
        {
            systemFatal("Unable to make thread to handle client");
        }
        
        /* Increment our count of connected clients and display it */
        displayClientData(connectedClients++);
    }
    
}

/*
 -- FUNCTION: processConnection
 --
 -- DATE: Feb 20, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int processConnection(int, int)
 --
 -- RETURNS: 1 on success
 --
 -- NOTES:
 -- Service a client socket by reading a request and sending the data to the
 -- client continueously in a loop until the client closes the connection.
 */
void *processConnection(void *data)
{
    int comm = (int) (long) data;
    int socket = (long) data >> sizeof(int);
    int bytesToWrite = 0;
    char line[NETWORK_BUFFER_SIZE];
    char result[NETWORK_BUFFER_SIZE];
    
    /* Ready the memory for sending to the client */
    memset(result, 'L', NETWORK_BUFFER_SIZE);
    
    /* Service the client while it is connected */
    while (1)
    {
        /* Read the request from the client */
        if (readLine(&socket, line, NETWORK_BUFFER_SIZE) <= 0)
        {
            close(socket);
            pthread_exit(NULL);
        }
        
        /* Get the number of bytes to reply with */
        bytesToWrite = atol(line);
        
        /* Ensure that the bytes requested are within our buffers */
        if ((bytesToWrite <= 0) || (bytesToWrite > NETWORK_BUFFER_SIZE))
        {
            systemFatal("Client requested too large a file");
        }
        
        /* Send the data back to the client */
        if (sendData(&socket, result, bytesToWrite) == -1)
        {
            systemFatal("Send fail");
        }
    }
}
                          
/*
 -- FUNCTION: initializeServer
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: September 22, 2011 - Added some extra comments about failure and
 -- a function call to set the socket into non blocking mode.
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: void initializeServer(int *listenSocket, int *port);
 --
 -- RETURNS: void
 --
 -- NOTES:
 -- This function sets up the required server connections, such as creating a
 -- socket, setting the socket to reuse mode, binding it to an address, and
 -- setting it to listen. If an error occurs, the function calls "systemFatal"
 -- with an error message.
 */
void initializeServer(int *listenSocket, int *port)
{
    // Create a TCP socket
    if ((*listenSocket = tcpSocket()) == -1)
    {
        systemFatal("Cannot Create Socket!");
    }
    
    // Allow the socket to be reused immediately after exit
    if (setReuse(listenSocket) == -1)
    {
        systemFatal("Cannot Set Socket To Reuse");
    }
    
    // Bind an address to the socket
    if (bindAddress(port, listenSocket) == -1)
    {
        systemFatal("Cannot Bind Address To Socket");
    }
    
    // Set the socket to listen for connections
    if (setListen(listenSocket) == -1)
    {
        systemFatal("Cannot Listen On Socket");
    }
}

void displayClientData(unsigned long long clients)
{
    printf("Connected clients: %llu\n", clients);
}

/*
 -- FUNCTION: systemFatal
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Aman Abdulla
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: static void systemFatal(const char* message);
 --
 -- RETURNS: void
 --
 -- NOTES:
 -- This function displays an error message and shuts down the program.
 */
static void systemFatal(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}
