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
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* User includes */
#include "network.h"

/* Client data struct define */
typedef struct
{
    char ip[16];
    int request;
    int port;
    int comm;
    unsigned int pause;
    unsigned long long maxRequests;
} clientData;

/* Function Protypes */
void *client(void* information);
void dataCollector(int socket);
void createClients(clientData data, int clients);
void stopClients();
void stopCollecting();
static void systemFatal(const char* message);

/* Main entry point */
int main(int argc, char **argv)
{
    /* Create variables and assign default data */
    int option = 0;
    int comms[2];
    int clients = 50;
    clientData data = {"192.168.0.189", 512, DEFAULT_PORT, 0, 5, 0};
    
    /* Get all the arguments */
    while ((option = getopt(argc, argv, "p:i:r:m:w:n:")) != -1)
    {
        switch (option) {
            case 'p':
                data.port = atoi(optarg);
                break;
            case 'i':
                snprintf(data.ip, sizeof(data.ip), "%s", optarg);
                break;
            case 'r': 
                data.request = atoi(optarg);
                break;
            case 'm':
                data.maxRequests = atoi(optarg);
                break;
            case 'w':
                data.pause = atoi(optarg);
                break;
            case 'n':
                clients = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s NEED TO DO USAGE\n", argv[0]);
                break;
        }
    }
    
    /* Create the socket pair for sending data for collection */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, comms) == -1)
    {
        systemFatal("Unable to create socket pair");
    }
    
    /* Create the data processing process and send it the other socket */
    if (!fork())
    {
        dataCollector(comms[1]);
        return 0;
    }
    
    /* Catch the SIGINT so we can shut down the data process */
    signal(SIGINT, stopClients);
    
    /* Assign the other socket to the thread data */
    data.comm = comms[0];
    
    /* Create the clients */
    createClients(data, clients);
    
    return 0;
    
}

void createClients(clientData data, int clients)
{
    int count = 0;
    clientData threadData[clients];
    pthread_t thread = 0;
    pthread_attr_t attr;
    
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
    
    for (count = 0; count < clients; count++)
    {
        memcpy(&threadData[count], &data, sizeof(clientData));
    }
    
    for (count = 0; count < clients; count++)
    {
        if (pthread_create(&thread, &attr, client, (void *) &threadData[count]) != 0)
        {
            systemFatal("Unable to make thread");
        }
    }
    
    pthread_attr_destroy(&attr);
    
    /* Wait for a signal from the data processing thread??? */
    wait(&count);
}

void *client(void *information)
{
    int socket = 0;
    unsigned long long count = 0;
    char *buffer = 0;
    char request[NETWORK_BUFFER_SIZE];
    clientData *data = (clientData *)information;
    
    /* Allocate memory and other setup */
    if ((buffer = malloc(sizeof(char) * NETWORK_BUFFER_SIZE)) == NULL)
    {
        systemFatal("Could not allocate buffer memory");
    }
    
    /* Convert the request size to a new line terminated string */
    snprintf(request, sizeof(request), "%d\n", data->request);
    
    /* Create the socket */
    if ((socket = tcpSocket()) == -1)
    {
        systemFatal("Could not create socket");
    }
    
    /* Set the socket to reuse for improper shutdowns */
    if (setReuse(&socket) == -1)
    {
        systemFatal("Could not set reuse");
    }
    
    /* Connect to the server */
    if (connectToServer(&data->port, &socket, data->ip) == -1)
    {
        systemFatal("Unable to connect to server");
    }
    
    /* Enter a loop and communicate with the server */
    while (1)
    {
        /* Get time before sending data */
        
        
        /* Send data */
        if (sendData(&socket, request, strlen(request)) == -1)
        {
            systemFatal("Unable to send data");
        }
        
        /* Receive data from the server */
        if (readData(&socket, buffer, data->request) == -1)
        {
            systemFatal("Unable to read data");
        }
        
        /* Get time after receiving response */
        
        
        /* Send data to the collection process */
        
        
        /* Increment count and check to see if we are done */
        if (data->maxRequests != 0)
        {
            if (++count >= data->maxRequests)
            {
                break;
            }
        }
        
        /* Wait the specified amount of time between requests */
        if (data->pause != 0)
        {
            sleep(data->pause);
        }
    }
    
    /* Clean up */
    if (closeSocket(&socket) == -1)
    {
        systemFatal("Unable to close socket");
    }
    if (closeSocket(&data->comm))
    {
        systemFatal("Unable to close socket");
    }
    free(buffer);
    
    
    pthread_exit(NULL);
}

void dataCollector(int socket)
{
    register char *buffer;
    register int fd = 0;
    unsigned long long requests = 0;
    unsigned long long dataSent = 0;
    
    signal(SIGINT, stopCollecting);
    
    if ((buffer = malloc(sizeof(char) * LOCAL_BUFFER_SIZE)) == NULL)
    {
        systemFatal("Could not allocate buffer memory");
    }
    
    if ((fd = open("clientData.txt", O_WRONLY | O_CREAT, 0666)) == -1)
    {
        systemFatal("Unable to create client data file");
    }
    
    while (1)
    {
        if (readData(&socket, buffer, LOCAL_BUFFER_SIZE) == -1)
        {
            systemFatal("Error reading client data");
        }
        
        if (write(fd, buffer, LOCAL_BUFFER_SIZE) == -1)
        {
            systemFatal("Unable to write client data to file");
        }
    }
    
    close(fd);
    close(socket);
}

void stopClients()
{
    printf("\nShutting down client process and threads\n");
    exit(0);
}

void stopCollecting()
{
    printf("\nClient data collection process is shutting down\n"); 
    exit(0);
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
