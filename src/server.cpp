#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "../include/server.h"

notepadServer::notepadServer()
{
    this->setupServerSocket();
    this->serve();
}

notepadServer::~notepadServer()
{
    fflush(stdout);
    for(int client: this->clients)
    {
        printf("Closing client connection with id: %d\n", client);
        fflush(stdout);
        close(client);
    }
    printf("Closing server connection with id: %d\n", this->socket);
    fflush(stdout);
    close(this->socket);
}

void notepadServer::setupServerSocket()
{
    if((this->socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation error.");
        exit(1);
    }

    memset(&this->serverAddr, 0, sizeof(this->serverAddr));
    this->serverAddr.sin_family = AF_INET;
    this->serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    this->serverAddr.sin_port = htons(4000);
    char option = 1;
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (void*) &option, sizeof(option));

    if(bind(this->socket, (struct sockaddr*) &this->serverAddr, sizeof(this->serverAddr)) == -1)
    {
        perror("Socket bind error.");
        exit(1);
    }

    if(listen(this->socket, 10) == -1)
    {
        perror("Listen error.");
        exit(1);
    }
}

void notepadServer::serve()
{
    while(1)
    {
        setupFileDescriptorSet();
        setupClients();

        select(this->maxDescriptor + 1, &this->readFDs, NULL, NULL, NULL);

        if(FD_ISSET(this->socket, &this->readFDs))
            serveServer();
        for(int client: this->clients)
            if(FD_ISSET(client, &readFDs))
                serveClient(client);
    }
}

void notepadServer::setupFileDescriptorSet()
{
    FD_ZERO(&this->readFDs);
    FD_SET(this->socket, &this->readFDs);
    this->maxDescriptor = this->socket;
}

void notepadServer::setupClients()
{
    for(int client: this->clients)
    {
        if(client > 0)
            FD_SET(client, &this->readFDs);
        if(client > this->maxDescriptor)
            this->maxDescriptor = client;
    }
}

void notepadServer::serveServer()
{
    int client;
    unsigned int len = sizeof(this->receiveAddr);
    if((client = ::accept(this->socket, (struct sockaddr*) &this->receiveAddr, &len)) == -1)
    {
        perror("Accept error.");
        exit(1);
    }

    printf("New client connected\n");
    fflush(stdout);

    this->clients.push_back(client);
}

void notepadServer::serveClient(int& socket)
{
    std::string message = this->readStringFromSocket(socket);
    if(message != "")
    {
        printf("Got message: %s\n", message.c_str());
        fflush(stdout);

        if(message == "Send")
            this->handleSendAllOpenFiles(socket);
        else if(message.find("Sending ") == 0)
            this->handleReceiveNewFile(socket, message);
        else if(message.find("Open ") == 0)
            this->handleSendOpenFile(socket, message);
    }
}

void notepadServer::handleSendAllOpenFiles(int& socket)
{
    printf("Sending data...\n");
    fflush(stdout);

    for(openFile* file: fileList)
        sendString(socket, (file->filename + file->isAvailable).c_str());
    sendEnd(socket);

    printf("Sent data...\n");
    fflush(stdout);
}

void notepadServer::handleReceiveNewFile(int& socket, std::string message)
{
    std::string content = readStringFromSocket(socket);
    std::string filename = message.substr(strlen("Sending "), message.length());

    printf("Filename: %s\n", filename.c_str());
    printf("File content: %s\n", content.c_str());
    fflush(stdout);

    sockaddr_in peerA;
    unsigned int len = sizeof(peerA);
    getpeername(socket, (sockaddr*) &peerA, &len);

    openFile* file = new openFile();
    file->filename = filename;
    file->filecontent = content;
    file->isAvailable = '+';
    file->peerA = peerA;
    file->peerB.sin_port = 0;

    fileList.push_back(file);
}

void notepadServer::handleSendOpenFile(int& socket, std::string message)
{
    bool found = false;
    openFile* reference;
    std::string filename = message.substr(strlen("Open "), message.length());

    for(openFile* file: fileList)
        if(file->filename == filename)
        {
            found = true;
            reference = file;
            break;
        }

    if(found)
    {
        if(reference->isAvailable == '+' && reference->peerB.sin_port == 0)
        {
            sendString(socket, reference->filename.c_str());
            sendString(socket, reference->filecontent.c_str());
            sendEnd(socket);

            sockaddr_in currentPeer;
            unsigned int lenCurrentPeer = sizeof(currentPeer);
            getpeername(socket, (sockaddr*) &currentPeer, &lenCurrentPeer);
            if(currentPeer.sin_addr.s_addr != reference->peerA.sin_addr.s_addr || \
               currentPeer.sin_port != reference->peerA.sin_port)
            {
                sockaddr_in peerB;
                unsigned int len = sizeof(peerB);
                getpeername(socket, (sockaddr*) &peerB, &len);
                reference->peerB = peerB;
                reference->isAvailable = '-';
            }
        }
        else
        {
            sendString(socket, "\0");
            sendString(socket, "\0");
            sendEnd(socket);
        }
    }
    else
    {
        sendString(socket, "\0");
        sendString(socket, "\0");
        sendEnd(socket);
    }
}

std::string notepadServer::readStringFromSocket(int& socket)
{
    std::string buffer = "";
    char currentChar;
    int bytesWritten;

    do
    {
        if((bytesWritten = ::read(socket, &currentChar, 1)) == -1)
        {
            perror("Read error.");
            exit(1);
        }
        else if(bytesWritten == 0)
        {
            sockaddr_in peer;
            unsigned int len = sizeof(peer);
            getpeername(socket, (sockaddr*)& peer, &len);
            for(openFile* file: this->fileList)
            {
                if(file->peerA.sin_addr.s_addr == peer.sin_addr.s_addr && \
                   file->peerA.sin_port == peer.sin_port)
                {
                   file->peerA.sin_addr.s_addr = 0;
                   file->peerA.sin_port = 0;
                }
                if(file->peerB.sin_addr.s_addr == peer.sin_addr.s_addr && \
                   file->peerB.sin_port == peer.sin_port)
                {
                   file->peerB.sin_addr.s_addr = 0;
                   file->peerB.sin_port = 0;
                }

                if(file->peerA.sin_addr.s_addr == 0 && file->peerA.sin_port == 0 && \
                   file->peerB.sin_addr.s_addr == 0 && file->peerB.sin_port == 0)
                {
                    this->fileList.erase(std::remove(this->fileList.begin(), this->fileList.end(), file), this->fileList.end());
                }
            }
            close(socket);
            socket = 0;
        }
        else
        {
            if(currentChar != 0)
                buffer += currentChar;
        }
    } while(currentChar != 0);

    return buffer;
}

void notepadServer::sendString(int& socket, std::string message)
{
    if(::write(socket, message.c_str(), message.length() + 1) == -1)
    {
        perror("Write error.");
        exit(1);
    }
}

void notepadServer::sendEnd(int& socket)
{
    if(::write(socket, "\0", 1) == -1)
    {
        perror("Write error.");
        exit(1);
    }
}
