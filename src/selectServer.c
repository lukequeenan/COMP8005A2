/*-----------------------------------------------------------------------------
 --	SOURCE FILE:    client.c - A simple TCP client and data collection program
 --
 --	PROGRAM:		NEED NAME
 --
 --	FUNCTIONS:		Berkeley Socket API
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
 -- A simple client emulation program for testing servers. The program
 -- implements the following features:
 --     1. Ability to send variable length text strings to the server
 --     2. Number of times to send these strings is user definable
 --     3. Have the client maintain the connection for varying time durations
 --     4. Keep track of how many requests it made to the server, amount of
 --        data sent to the server, amount of time it took for the server to
 --        respond
 --
 -- This program will also allow the user to specify the number of above
 -- clients to spawn via threads. A process is also created that will collect
 -- any statistical data and save it to a file. This is done using UNIX domain
 -- sockets.
 ----------------------------------------------------------------------------*/

/* System includes */
#include <stdlib.h>
#include <stdio.h>
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

void server(int port, int comm)
{
    int listenSocket = 0;
    int client = 0;
    int maxFileDescriptor = 0;
    int index = 0;
    fd_set clients;
    fd_set activeClients;
    unsigned long long connections = 0;
    
    /* Initialize the server */
    initializeServer(&listenSocket, &port);
    
    /* Set up select variables */
    FD_ZERO(&clients);
    FD_ZERO(&activeClients);
    FD_SET(listenSocket, &clients);
    maxFileDescriptor = listenSocket;
    
    while (1)
    {
        displayClientData(connections);
        activeClients = clients;
        if (select(maxFileDescriptor + 1, &activeClients,
                    NULL, NULL, NULL) == -1)
        {
            systemFatal("Error with pselect");
        }
        
        /* First check for a new client connection */
        if (FD_ISSET(listenSocket, &activeClients))
        {
            /* Accept the new connection */
            if ((client = acceptConnection(&listenSocket)) != -1)
            {
                FD_SET(client, &clients);
                connections++;
                if (client > maxFileDescriptor)
                {
                    maxFileDescriptor = client;
                }
            }
        }
        
        /* Now process any requests made by the other connected clients */
        for (index = 0; index <= maxFileDescriptor; index++)
        {
            if (FD_ISSET(index, &activeClients))
            {
                if (index != listenSocket)
                {
                    if (processConnection(index, comm) == 0)
                    {
                        FD_CLR(index, &clients);
                        close(index);
                        connections--;
                    }
                }
            }
        }
    }
    
    close(listenSocket);
}

int processConnection(int socket, int comm)
{
    int bytesToWrite = 0;
    int count = 0;
    char line[NETWORK_BUFFER_SIZE];
    char result[NETWORK_BUFFER_SIZE];
    
    /* Read the request from the client */
    if ((count = readLine(&socket, line, NETWORK_BUFFER_SIZE)) == -1)
    {
        systemFatal("Unable to read from client");
    }
    else if (count == 0)
    {
        return 0;
    }
    
    /* Get the number of bytes to reply with */
    bytesToWrite = atol(line);
    
    for (count = 0; count < bytesToWrite; count++)
    {
        result[count] = (char)count;
    }
    
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
    
    // Set the socket to listen for connections
    if (setListen(listenSocket) == -1)
    {
        systemFatal("Cannot Listen On Socket");
    }
}

void displayClientData(unsigned long long clients)
{/*
    if (system("clear") == -1)
    {
        systemFatal("Clear screen failed");
    }
    printf("\n\nConnected clients: %llu\n", clients);*/
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
