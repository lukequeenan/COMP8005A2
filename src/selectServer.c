/*-----------------------------------------------------------------------------
 --	SOURCE FILE:    selectServer.c - A simple select server program
 --
 --	PROGRAM:		NEED NAME
 --
 --	FUNCTIONS:		
 --                 int main(int argc, char **argv);
 --                 void server(int port, int comm);
 --                 int processConnection(int socket, int comm);
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
 -- A simple select server
 ----------------------------------------------------------------------------*/

/* System includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <unistd.h>

/* User includes */
#include "network.h"

int main(int argc, char **argv);
void server(int port, int comm);
int processConnection(int socket, int comm);
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
 -- This is the main entry point for the select server
 */
int main(int argc, char **argv)
{
    /* Initialize port and give default option in case of no user input */
    int port = DEFAULT_PORT;
    int option = 0;
    int comms[2];
    
    /* Parse command line parameters using getopt */
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
    
    /* Create the socket pair for sending data for collection */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, comms) == -1)
    {
        systemFatal("Unable to create socket pair");
    }
    
    /* Need to fork and create process to collect data */
    
    /* Start server */
    server(port, comms[1]);
    
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
 -- INTERFACE: void server(int, int)
 --
 -- RETURNS: void
 --
 -- NOTES:
 -- This function contains the server loop for select. It accept client
 -- connections and calls the process connection function when a socket is ready
 -- for reading.
 */
void server(int port, int comm)
{
    int listenSocket = 0;
    register int client = 0;
    register int index = 0;
    fd_set clients;
    fd_set activeClients;
    unsigned long long connections = 0;
    
    /* Initialize the server */
    initializeServer(&listenSocket, &port);
    
    /* Set up select variables */
    FD_ZERO(&clients);
    FD_ZERO(&activeClients);
    FD_SET(listenSocket, &clients);
    
    displayClientData(connections);
    
    while (1)
    {
        activeClients = clients;
        if (select(FD_SETSIZE, &activeClients, NULL, NULL, NULL) == -1)
        {
            systemFatal("Error with select");
        }
        
        /* Process all the sockets */
        for (index = 0; index < FD_SETSIZE; index++)
        {
            if (FD_ISSET(index, &activeClients))
            {
                if (index != listenSocket)
                {
                    if (processConnection(index, comm) == 0)
                    {
                        close(index);
                        FD_CLR(index, &clients);
                        connections--;
                        displayClientData(connections);
                    }
                }
                else
                {
                    /* Accept the new connections */
                    while ((client = acceptConnection(&listenSocket)) != -1)
                    {
                        FD_SET(client, &clients);
                        connections++;
                        displayClientData(connections);
                    }
                }
            }
        }
    }
    
    close(listenSocket);
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
 -- client.
 */
int processConnection(int socket, int comm)
{
    int bytesToWrite = 0;
    char line[NETWORK_BUFFER_SIZE];
    char result[NETWORK_BUFFER_SIZE];
    
    /* Ready the memory for sending to the client */
    memset(result, 'L', NETWORK_BUFFER_SIZE);
    
    /* Read the request from the client */
    if (readLine(&socket, line, NETWORK_BUFFER_SIZE) <= 0)
    {
        return 0;
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

    /* Send the communication time to the data collection process */
    
    
    return 1;
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
    
    if (makeSocketNonBlocking(listenSocket) == -1)
    {
        systemFatal("Cannot Make Socket Non-Blocking");
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
