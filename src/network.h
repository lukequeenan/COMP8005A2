#ifndef NETWORK_H
#define NETWORK_H

/* Defines */
#define NETWORK_BUFFER_SIZE 1024
#define LOCAL_BUFFER_SIZE 1024
#define DEFAULT_PORT 8989

/* Function Prototypes */
#ifdef __cplusplus
extern "C" {
#endif
    int tcpSocket();
    int setReuse(int* socket);
    int bindAddress(int *port, int *socket);
    int setListen(int *socket);
    int acceptConnection(int *listenSocket);
    int acceptConnectionIp(int *listenSocket, char* ip);
    int acceptConnectionIpPort(int *listenSocket, char *ip, unsigned short *port);
    int readData(int *socket, char *buffer, int bytesToRead);
    int sendData(int *socket, const char *buffer, int bytesToSend);
    int readLine(int *socket, char *buffer, int maxBytesToRead);
    int closeSocket(int *socket);
    int connectToServer(const char *port, int *socket, const char *ip);
    int makeSocketNonBlocking(int *socket);
#ifdef __cplusplus
}
#endif
#endif
