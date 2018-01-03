#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/declarations.h"

void setupServerSocket(int& serverSock, struct sockaddr_in& serverAddr, char& option);
void resetClientArray(int* array, int size);
void serve(int& serverSocket, struct sockaddr_in& receiveAddr, int* clients, int& maxClients);
void setupFileDescriptorSet(fd_set& readFDs, int serverSocket, int& maxDescriptor);
void setupClients(int* clients, int maxClients, fd_set& readFDs, int& maxDescriptor);
void serveServer(int& serverSocket, struct sockaddr_in& receiveAddr, unsigned int& len, int* clients, int& maxClients);
void serveClient(int& clients, fd_set& readFDs, std::vector<openFile*> &fileList);

#endif
