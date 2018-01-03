#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../include/server.h"
#include "../include/declarations.h"
#include "../include/network.h"
#include "../include/handlers.h"

void setupServerSocket(int& serverSocket, struct sockaddr_in& serverAddr, char& option)
{
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation error.");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(4000);

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (void*) &option, sizeof(option));

    if(bind(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Socket bind error.");
        exit(1);
    }

    if(listen(serverSocket, 10) == -1)
    {
        perror("Listen error.");
        exit(1);
    }
}

void resetClientArray(int* array, int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
        array[i] = 0;
    }
}

void serve(int& serverSocket, struct sockaddr_in& receiveAddr, int* clients, int& maxClients)
{
    fd_set readFDs;
    int i;
    int maxDescriptor;
    unsigned int len;
    std::vector<openFile*> fileList;

    len = sizeof(&receiveAddr);
    while(1)
    {
        setupFileDescriptorSet(readFDs, serverSocket, maxDescriptor);
        setupClients(clients, maxClients, readFDs, maxDescriptor);

        select(maxDescriptor + 1, &readFDs, NULL, NULL, NULL);

        if(FD_ISSET(serverSocket, &readFDs))
            serveServer(serverSocket, receiveAddr, len, clients, maxClients);
        for(i = 0; i < maxClients; i++)
            if(FD_ISSET(clients[i], &readFDs))
                serveClient(clients[i], readFDs, fileList);
    }
}

void setupFileDescriptorSet(fd_set& readFDs, int serverSocket, int& maxDescriptor)
{
    FD_ZERO(&readFDs);
    FD_SET(serverSocket, &readFDs);
    maxDescriptor = serverSocket;
}

void setupClients(int* clients, int maxClients, fd_set& readFDs, int& maxDescriptor)
{
    int i;
    for(i = 0; i < maxClients; i++)
    {
        if(clients[i] > 0)
            FD_SET(clients[i], &readFDs);
        if(clients[i] > maxDescriptor)
            maxDescriptor = clients[i];
    }
}

void serveServer(int& serverSocket, struct sockaddr_in& receiveAddr, unsigned int& len, int* clients, int& maxClients)
{
    int i;
    int client;
    if((client = accept(serverSocket, (struct sockaddr*) &receiveAddr, &len)) == -1)
    {
        perror("Accept error.");
        exit(1);
    }

    printf("New client connected\n");
    fflush(stdout);
    for(i = 0; i < maxClients; i++)
        if(clients[i] == 0)
        {
            clients[i] = client;
            break;
        }
}

void serveClient(int& socket, fd_set& readFDs, std::vector<openFile*>& fileList)
{
    std::string message = readStringFromSocket(socket);

    printf("Got message: %s\n", message.c_str());
    fflush(stdout);

    if(message == "Send")
        handleSendAllOpenFiles(socket, fileList);
    else if(message.find("Sending ") == 0)
        handleReceiveNewFile(socket, message, fileList);
    else if(message.find("Open ") == 0)
        handleSendOpenFile(socket, message, fileList);
}
