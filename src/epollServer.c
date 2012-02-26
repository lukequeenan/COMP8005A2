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
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <unistd.h>

#include <pthread.h>

/* User includes */
#include "network.h"

#define MAX_EVENTS 10000
#define THREADS 64

int main(int argc, char **argv);
void server(int port, int comm);
void *processConnection();
void initializeServer(int *listenSocket, int *port);
void createThreadPool(int threadsToMake);
void displayClientData(unsigned long long clients);
static void systemFatal(const char *message);

typedef struct
{
    int socket;
    int commSocket;
} clientData;

static int threadSocket = 0;
pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clientCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t epollLoop = PTHREAD_COND_INITIALIZER;
pthread_mutex_t acceptMutex = PTHREAD_MUTEX_INITIALIZER;


int main(int argc, char **argv)
{
    /* Initialize port and give default option in case of no user input */
    int port = DEFAULT_PORT;
    int threads = THREADS;
    int option = 0;
    int comms[2];
    
    /* Parse command line parameters using getopt */
    while ((option = getopt(argc, argv, "p:t:")) != -1)
    {
        switch (option)
        {
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                threads = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -p [port] -t [threads]\n", argv[0]);
                return 0;
        }
    }
    
    /* Create the socket pair for sending data for collection */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, comms) == -1)
    {
        systemFatal("Unable to create socket pair");
    }
    
    /* Need to fork and create process to collect data */
    
    /* Create thread pool */
    createThreadPool(threads);
    
    /* Start server */
    server(port, comms[1]);
    
    return 0;
}

void server(int port, int comm)
{
    register int epoll = 0;
    register int ready = 0;
    register int index = 0;
    int listenSocket = 0;
    int client = 0;
    
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];
    unsigned long long connections = 0;
    
    /* Initialize the server */
    initializeServer(&listenSocket, &port);
    
    /* Set up epoll variables */
    if ((epoll = epoll_create1(0)) == -1)
    {
        systemFatal("Unable to create epoll object");
    }
    
    event.events = EPOLLIN;
    event.data.fd = listenSocket;
    
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, listenSocket, &event) == -1)
    {
        systemFatal("Unable to add listen socket to epoll");
    }
    
    displayClientData(connections);
    
    while (1)
    {
        /* Wait for epoll to return with the maximum events specified */
        ready = epoll_wait(epoll, events, MAX_EVENTS, -1);
        if (ready == -1)
        {
            systemFatal("Epoll wait error");
        }
        
        /* Iterate through the returned sockets and deal with them */
        for (index = 0; index < ready; index++)
        {
            if (events[index].data.fd == listenSocket)
            {
                /* Accept the new connections */
                while ((client = acceptConnection(&listenSocket)) != -1)
                {
                    if (makeSocketNonBlocking(&client) == -1)
                    {
                        systemFatal("Cannot make client socket non-blocking");
                    }
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = client;
                    if (epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event) == -1)
                    {
                        systemFatal("Cannot add client socket to epoll");
                    }
                    connections++;
                    displayClientData(connections);
                }
            }
            else
            {
                pthread_mutex_lock(&clientMutex);
                if (threadSocket != 0)
                {
                    pthread_cond_wait(&epollLoop, &clientMutex);
                }
                threadSocket = events[index].data.fd;
                pthread_cond_signal(&clientCondition);
                pthread_mutex_unlock(&clientMutex);
                
                displayClientData(connections++);
            }
        }
    }
    
    close(listenSocket);
    close(epoll);
}

void *processConnection()
{
    int socket = 0;
    register int bytesToWrite = 0;
    char line[NETWORK_BUFFER_SIZE];
    char result[NETWORK_BUFFER_SIZE];
    
    /* Ready the memory for sending to the client */
    memset(result, 'L', NETWORK_BUFFER_SIZE);
    
    while (1)
    {
        /* Wait on the client mutex */ 
        pthread_mutex_lock(&clientMutex);
        
        /* If there are no clients to process, wait until there are */
        if (threadSocket == 0)
        {
            pthread_cond_wait(&clientCondition, &clientMutex);
        }
        
        /* Copy the socket to a local variable and set global to 0 */
        socket = threadSocket;
        threadSocket = 0;
        pthread_cond_signal(&epollLoop);
        
        /* Release the mutex, and process the client */
        pthread_mutex_unlock(&clientMutex);
        
        /* Read the request from the client */
        if (readLine(&socket, line, NETWORK_BUFFER_SIZE) <= 0)
        {
            close(socket);
            continue;
        }
        
        /* Get the number of bytes to reply with */
        bytesToWrite = atol(line);
        
        /* Ensure that the bytes requested are within our buffers */
        if ((bytesToWrite <= 0) || (bytesToWrite > NETWORK_BUFFER_SIZE))
        {
            close(socket);
            continue;
        }

        /* Send the data back to the client */
        if (sendData(&socket, result, bytesToWrite) == -1)
        {
            close(socket);
            continue;
        }
    }
    
    pthread_exit(NULL);
}

void createThreadPool(int threadsToMake)
{
    int count = 0;
    pthread_t thread = 0;
    pthread_attr_t attr;
    
    threadSocket = 0;
    
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
    
    /* Create the threads */
    for (count = 0; count < threadsToMake; count++)
    {
        if (pthread_create(&thread, &attr, processConnection, NULL) != 0)
        {
            systemFatal("Unable to make thread in thread pool");
        }
    }
    
    /* Destroy thread attributes */
    pthread_attr_destroy(&attr);
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
