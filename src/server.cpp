#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <sstream>
#include <iostream>

#include "../include/server.h"

notepadServer::notepadServer()
{
    this->setupServerSocket();
    this->getExistingFileList();
    this->serve();
}

notepadServer::~notepadServer()
{
    printf("Closing server socket\n");
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

void notepadServer::getExistingFileList()
{
    //TODO: implement this method;
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
    Json::Value object;
    std::string message = this->readStringFromSocket(socket);
    std::stringstream(message) >> object;

    if(message != "")
    {
        printf("Got message: %s\n", message.c_str());
        fflush(stdout);

        if(object["reason"] == "Send all files")
            this->handleSendAllOpenFiles(socket);
        else if(object["reason"] == "Sending file")
            this->handleReceiveNewFile(socket, object);
        else if(object["reason"] == "Open file")
            this->handleSendOpenFile(socket, object);
        else if(object["reason"] == "Edit file")
            this->handleEditOpenFile(socket, object);
    }
}

void notepadServer::handleSendAllOpenFiles(int& socket)
{
    printf("Sending data...\n");
    fflush(stdout);

    Json::Value array;
    Json::Value object;

    for(openFile* file: fileList)
        array.append(file->filename + file->isAvailable);

    object["reason"] = "Send all files";
    object["files"] = array;
    if(!array.isNull())
    {
        printf("Built array.\n");
        fflush(stdout);

        std::string output;
        Json::FastWriter writer;
        output = writer.write(object);

        sendString(socket, output.c_str());
    }
    else
    {
        Json::Value newObject;
        newObject["reason"] = "Send all files";
        newObject["files"] = Json::arrayValue;

        std::string output;
        Json::FastWriter writer;
        output = writer.write(newObject);

        sendString(socket, output.c_str());
    }

    printf("Sent data...\n");
    fflush(stdout);
}

void notepadServer::handleReceiveNewFile(int& socket, Json::Value object)
{
    std::string filename = object["filename"].asString();
    std::string content = object["filecontent"].asString();

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
    file->socketA = socket;
    file->peerB.sin_port = 0;
    file->socketB = -1;

    fileList.push_back(file);
}

void notepadServer::handleSendOpenFile(int& socket, Json::Value object)
{
    bool found = false;
    openFile* reference;
    std::string filename = object["filename"].asString();

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
            Json::Value newObject;
            newObject["reason"] = "Open file";
            newObject["filename"] = reference->filename;
            newObject["filecontent"] = reference->filecontent;

            std::string output;
            Json::FastWriter writer;
            output = writer.write(newObject);

            sendString(socket, output.c_str());

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
                reference->socketB = socket;
                reference->isAvailable = '-';
            }
        }
        else
        {
            Json::Value newObject;
            newObject["reason"] = "Open file";
            newObject["filename"] = reference->filename;
            newObject["filecontent"] = reference->filecontent;

            std::string output;
            Json::FastWriter writer;
            output = writer.write(newObject);

            sendString(socket, output.c_str());
        }
    }
    else
    {
        Json::Value newObject;
        newObject["reason"] = "Open file";
        newObject["filename"] = reference->filename;
        newObject["filecontent"] = reference->filecontent;

        std::string output;
        Json::FastWriter writer;
        output = writer.write(newObject);

        sendString(socket, output.c_str());
    }
}

void notepadServer::handleEditOpenFile(int &socket, Json::Value object)
{
    std::string output;
    Json::FastWriter writer;
    output = writer.write(object);
    bool found = false;
    openFile* reference;
    std::string filename = object["filename"].asString();
    std::string replacement = object["filecontent"].asString();

    for(openFile* file: fileList)
        if(file->filename == filename)
        {
            found = true;
            reference = file;
            break;
        }

    sockaddr_in peer;
    unsigned int len = sizeof(peer);
    getpeername(socket, (sockaddr*)& peer, &len);

    if(found)
    {
        if(peer.sin_addr.s_addr == reference->peerA.sin_addr.s_addr &&
           peer.sin_port == reference->peerA.sin_port)
        {
            reference->filecontent = replacement;
            if(reference->peerB.sin_port != 0 && reference->socketB != -1)
            {
                sendString(reference->socketB, output);
            }
        }
        else if(peer.sin_addr.s_addr == reference->peerB.sin_addr.s_addr &&
            peer.sin_port == reference->peerB.sin_port)
        {
            reference->filecontent = replacement;
            if(reference->peerA.sin_port != 0 && reference->socketA != -1)
            {
                sendString(reference->socketA, output);
            }
        }
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
