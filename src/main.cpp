#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/server.h"

int serverSocket;

void signalHandler(int sig)
{
    if(serverSocket != 0)
    {
        close(serverSocket);
    }
}

int main(int argc, char** argv)
{
    sockaddr_in serverAddr;
    sockaddr_in receiveAddr;

    char option = 1;
    int clients[50];
    int maxClients = 50;

    signal(SIGINT, signalHandler);
    setupServerSocket(serverSocket, serverAddr, option);
    resetClientArray(clients, maxClients);
    serve(serverSocket, receiveAddr, clients, maxClients);

    return 0;
}
