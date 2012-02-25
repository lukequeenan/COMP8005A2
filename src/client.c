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
#include <sys/time.h>
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
    int clients;
    unsigned int pause;
    unsigned long long maxRequests;
} threadData;

/* Function Protypes */
void *client(void* information);
void dataCollector(int socket, int clients);
void createClients(threadData data, int threads);
void stopClients();
void stopCollecting();
static void systemFatal(const char* message);

/* Main entry point */
int main(int argc, char **argv)
{
    /* Create variables and assign default data */
    int option = 0;
    int comms[2];
    int threads = 8;
    threadData data = {"192.168.0.175", 1024, DEFAULT_PORT, 0, 10, 0, 100};
    
    /* Get all the arguments */
    while ((option = getopt(argc, argv, "p:i:r:m:w:n:t:")) != -1)
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
                data.clients = atoi(optarg);
                break;
            case 't':
                threads = atoi(optarg);
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
        dataCollector(comms[1], threads);
        return 0;
    }
    
    /* Catch the SIGINT so we can shut down the data process */
    signal(SIGINT, stopClients);
    
    /* Assign the other socket to the thread data */
    data.comm = comms[0];
    
    /* Create the clients */
    createClients(data, threads);
    
    return 0;
    
}

void createClients(threadData clientData, int threads)
{
    int count = 0;
    threadData data[threads];
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
    
    for (count = 0; count < threads; count++)
    {
        memcpy(&data[count], &clientData, sizeof(threadData));
    }
    
    for (count = 0; count < threads; count++)
    {
        if (pthread_create(&thread, &attr, client, (void *) &data[count]) != 0)
        {
            systemFatal("Unable to make thread");
        }
    }
    
    pthread_attr_destroy(&attr);
    
    /* Wait for a signal from the data processing process */
    wait(&count);
}

void *client(void *information)
{
    int read = 0;
    int result = 0;
    int *sockets = 0;
    register int index = 0;
    unsigned long long count = 0;
    unsigned long long dataReceived = 0;
    unsigned long long requestTime = 0;
    char *buffer = 0;
    char request[NETWORK_BUFFER_SIZE];
    struct timeval startTime;
    struct timeval endTime;
    threadData *data = (threadData *)information;
    
    /* Allocate memory and other setup */
    if ((buffer = malloc(sizeof(char) * NETWORK_BUFFER_SIZE)) == NULL)
    {
        systemFatal("Could not allocate buffer memory");
    }
    if ((sockets = malloc(sizeof(int) * data->clients)) == NULL)
    {
        systemFatal("Could not allocate socket memory");
    }
    
    /* Convert the request size to a new line terminated string */
    snprintf(request, sizeof(request), "%d\n", data->request);
    
    for (index = 0; index < data->clients; index++)
    {
        /* Create the socket */
        if ((sockets[index] = tcpSocket()) == -1)
        {
            systemFatal("Could not create socket");
        }
        
        /* Set the socket to reuse for improper shutdowns */
        if (setReuse(&sockets[index]) == -1)
        {
            systemFatal("Could not set reuse");
        }
        
        /* Connect to the server */
        result = connectToServer(&data->port, &sockets[index], data->ip);
    }
    
    /* Connect to the server */
    if (result != -1)
    {
        /* Enter a loop and communicate with the server */
        while (1)
        {
            count++;
            
            for (index = 0; index < data->clients; index++)
            {
                /* Get time before sending data */
                gettimeofday(&startTime, NULL);
                
                /* Send data */
                if (sendData(&sockets[index], request, strlen(request)) == -1)
                {
                    continue;
                }

                /* Receive data from the server */
                if ((read = readData(&sockets[index], buffer, data->request)) == -1)
                {
                    continue;
                }
                
                /* Get time after receiving response */
                gettimeofday(&endTime, NULL);
                
                /* Save data */
                dataReceived += read;
                requestTime += (endTime.tv_sec * 1000000 + endTime.tv_usec) -
                               (startTime.tv_sec * 1000000 + startTime.tv_usec);
            }

            /* Increment count and check to see if we are done */
            if (count >= data->maxRequests)
            {
                break;
            }
            
            /* Wait the specified amount of time between requests */
            if (data->pause != 0)
            {
                sleep(data->pause);
            }
        }
    }
    
    /* Clean up */
    for (index = 0; index < data->clients; index++)
    {
        if (closeSocket(&sockets[index]) == -1)
        {
            systemFatal("Unable to close socket");
        }
    }
    
    /* Create and format data for output */
    snprintf(request, sizeof(request), "Requests: %llu, Request Time: %llu, "
             "Data Received: %llu\n", count, requestTime, dataReceived);
    
    /* Send data to comms process */
    if (sendData(&data->comm, request, strlen(request)) == -1)
    {
        systemFatal("Unable to send result data");
    }
    
    free(buffer);
    
    pthread_exit(NULL);
}

void dataCollector(int socket, int clients)
{
    char *buffer;
    int fd = 0;
    int count = 0;
    int clientCount = 0;
    
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
        if ((count = readLine(&socket, buffer, LOCAL_BUFFER_SIZE)) == -1)
        {
            systemFatal("Error reading client data");
        }
        
        if (write(fd, buffer, ++count) == -1)
        {
            systemFatal("Unable to write client data to file");
        }
        
        if (++clientCount >= clients)
        {
            break;
        }
    }
    
    close(fd);
    close(socket);
    exit(0);
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
