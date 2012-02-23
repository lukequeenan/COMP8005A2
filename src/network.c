/*
 -- SOURCE FILE: network.c
 --
 -- PROGRAM: Pelican Chat Program
 --
 -- FUNCTIONS:
 -- int tcpSocket();
 -- int setReuse(int* socket);
 -- int bindAddress(int *port, int *socket);
 -- int setListen(int *socket);
 -- int acceptConnection(int *listenSocket);
 -- int readData(int *socket, char *buffer, int bytesToRead);
 -- int sendData(int *socket, char *buffer, int bytesToSend);
 -- int closeSocket(int *socket);
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- NOTES:
 -- This file contains generic network wrapper functions for use by C or C++
 -- programs.The file contains all network related header files, meaning that the
 -- user of this library does not need to include anything except the network.h
 -- header file.
 */

// Includes
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "network.h"

#define MAX_QUEUE 10

/*
 -- FUNCTION: tcpSocket
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int tcpSocket();
 --
 -- RETURNS: a new tcp socket
 --
 -- NOTES:
 -- This is the wrapper function for creating a new tcp socket.
 */
int tcpSocket()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

/*
 -- FUNCTION: setReuse
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int setReuse(int *socket);
 --
 -- RETURNS: the result of the setsockopt function
 --
 -- NOTES:
 -- This is the wrapper function for setting the reuse option on a socket. This
 -- allows the socket to be reused after an improper shutdown.
 */
int setReuse(int *socket)
{
    socklen_t optlen = 1;
    return setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &optlen,
                      sizeof(optlen));
}

/*
 -- FUNCTION: bindAddress
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int bindAddress(int *port, int *socket);
 --
 -- RETURNS: the result of the bind function
 --
 -- NOTES:
 -- This is the wrapper function for binding an address to a socket.
 */
int bindAddress(int *port, int *socket)
{
    struct sockaddr_in address;
    
    bzero((char *)&address, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(*port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    
    return bind(*socket, (struct sockaddr *)&address, sizeof(address));
}

/*
 -- FUNCTION: setListen
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int setListen(int *socket);
 --
 -- RETURNS: the result of listen function
 --
 -- NOTES:
 -- This is the wrapper function for setting a socket to listen on.
 */
int setListen(int *socket)
{
    return listen(*socket, MAX_QUEUE);
}

/*
 -- FUNCTION: acceptConnection
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int acceptConnection(int *listenSocket);
 --
 -- RETURNS: the new socket created for the connection
 --
 -- NOTES:
 -- This is the wrapper function for accepting a connection from the specified
 -- socket.
 */
int acceptConnection(int *listenSocket)
{
    struct sockaddr_in clientAddress;
    socklen_t addrlen = sizeof(clientAddress);
    return accept(*listenSocket, (struct sockaddr *) &clientAddress, &addrlen);
}

/*
 -- FUNCTION: acceptConnectionIp
 --
 -- DATE: March 12, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int acceptConnectionIp(int *listenSocket, char *ip);
 --
 -- RETURNS: the new socket created for the connection
 --
 -- NOTES:
 -- This is the wrapper function for accepting a connection from the specified
 -- socket. If you need the client's ip address, you can pass a char* to the
 -- function. You must ensure that the length of the ip array is long enough.
 */
int acceptConnectionIp(int *listenSocket, char *ip)
{
    int sock = 0;
    struct sockaddr_in clientAddress;
    socklen_t addrlen = sizeof(clientAddress);
    sock = accept(*listenSocket, (struct sockaddr *) &clientAddress, &addrlen);
    strcpy(ip, inet_ntoa(clientAddress.sin_addr));
    return sock;
}

/*
 -- FUNCTION: acceptConnectionPort
 --
 -- DATE: September 28, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int acceptConnectionIpPort(int *listenSocket, char *ip,
 --                                       unsigned short *port);
 --
 -- RETURNS: the new socket created for the connection
 --
 -- NOTES:
 -- This is the wrapper function for accepting a connection from the specified
 -- socket. If you need the client's ip address, you can pass a char* to the
 -- function. You must ensure that the length of the ip array is long enough. If
 -- you need the client's port, you can pass an unsigned short* to the function.
 */
int acceptConnectionIpPort(int *listenSocket, char *ip, unsigned short *port)
{
    int sock = 0;
    struct sockaddr_in clientAddress;
    socklen_t addrlen = sizeof(clientAddress);
    sock = accept(*listenSocket, (struct sockaddr *) &clientAddress, &addrlen);
    strcpy(ip, inet_ntoa(clientAddress.sin_addr));
    *port = htons(clientAddress.sin_port);
    return sock;
}

/*
 -- FUNCTION: readData
 --
 -- DATE: March 13, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int readData(int *socket, char *buffer, int bytesToRead);
 --
 -- RETURNS: the number of bytes read
 --
 -- NOTES:
 -- This is the wrapper function for reading data from a socket. The data is
 -- stored in a char array. The function will continue to read until it has read
 -- the number of bytes specified by bytesToRead. You MUST ensure that the sender
 -- has sent the SAME number of bytes, or this function will block.
 */
int readData(int *socket, char *buffer, int bytesToRead)
{    
    return recv(*socket, buffer, bytesToRead, MSG_WAITALL);
}

/*
 -- FUNCTION: sendData
 --
 -- DATE: March 13, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int sendData(int *socket, const char *buffer, int bytesToSend);
 --
 -- RETURNS: the bytes written to the specified socket
 --
 -- NOTES:
 -- This is the wrapper function for sending a char buffer to a socket using
 -- write.
 */
int sendData(int *socket, const char *buffer, int bytesToSend)
{
    return send(*socket, buffer, bytesToSend, 0);
}

/*
 -- FUNCTION: readLine
 --
 -- DATE: February 11, 2012
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int readLine(int *socket, char *buffer, int maxBytesToRead);
 --
 -- RETURNS: the number of bytes read
 --
 -- NOTES:
 -- This is the wrapper function for reading a line from a socket. The function
 -- will continue to read from the socket until the maximum bytes to read has
 -- been met, or the buffer reaches a new line.
 */
int readLine(int *socket, char *buffer, int maxBytesToRead)
{
    int count = 0;
    int bytesRead = 0;
    char c = 0;
    
    for (count = 1; count < maxBytesToRead; count++)
    {
        if ((bytesRead = read(*socket, &c, 1)) == 1)
        {
            *buffer++ = c;
            /* Get out of the read if we find the new line */
            if (c == '\n')
            {
                break;
            }
        }
        else if (bytesRead == 0)
        {
            /* EOF, no data read */
            if (count == 1)
            {
                return 0;
            }
            else
            {
                break;
            }
        }
        else
        {
            return -1;
        }
    }
    
    return count;
}

/*
 -- FUNCTION: closeSocket
 --
 -- DATE: March 13, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int closeSocket(int *socket);
 --
 -- RETURNS: the result of the close function
 --
 -- NOTES:
 -- This is the wrapper function for closing a file descriptor.
 */
int closeSocket(int *socket)
{
    return close(*socket);
}

/*
 -- FUNCTION: connectToServer
 --
 -- DATE: March 14, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int connectToServer(int *port, int *socket, const char *ip);
 --
 -- RETURNS: the result of the connect function or gethostbyname if it fails
 --
 -- NOTES:
 -- This is the wrapper function for connecting to a server.
 */
int connectToServer(int *port, int *socket, const char *ip)
{
    struct sockaddr_in address;
    struct hostent *hp;
    
    bzero((char *)&address, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(*port);
    if ((hp = gethostbyname(&(*ip))) == NULL)
    {
        return -1;
    }
    bcopy(hp->h_addr, (char *)&address.sin_addr, hp->h_length);
    
    return connect(*socket, (struct sockaddr *)&address, sizeof(address));
}

/*
 -- FUNCTION: makeSocketNonBlocking
 --
 -- DATE: September 22, 2011
 --
 -- REVISIONS: (Date and Description)
 --
 -- DESIGNER: Luke Queenan
 --
 -- PROGRAMMER: Luke Queenan
 --
 -- INTERFACE: int makeSocketNonBlocking(int *socket);
 --
 -- RETURNS: the result of the fcntl function
 --
 -- NOTES:
 -- This is the wrapper function for setting the non blocking flag on the socket
 -- desriptor.
 */
int makeSocketNonBlocking(int *socket)
{
    int flags = 0;
    
    // Get the current flags off the socket
    flags = fcntl(*socket, F_GETFL, 0);
    
    // Make sure we did not encounter an error
    if (flags == -1)
    {
        return -1;
    }
    
    // Set the flags to non blocking
    flags |= O_NONBLOCK;
    
    return fcntl(*socket, F_SETFL, flags);
}
