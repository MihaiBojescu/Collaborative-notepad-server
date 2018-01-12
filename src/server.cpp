#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <json/json.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

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
    std::cout << "Closing server socket." << std::endl;
    for(int client: this->clients)
    {
        std::cout << "Closing client connection with id: " << client << std::endl;
        close(client);
    }
    std::cout << "Closing server socket with id: " << this->socket << std::endl;
    close(this->socket);
}

void notepadServer::getExistingFileList()
{
    DIR* directory;
    dirent* directoryEntries;
    char currentChar;
    bool noError;
    int file;
    int status;

    if((directory = opendir(".")) == NULL)
    {
        perror("Can't open current dir!");
        exit(1);
    }
    else
    {
        while((directoryEntries = readdir(directory)) != NULL)
        {
            if(directoryEntries->d_type != DT_DIR)
            {
                openFile* newFile = new openFile();
                newFile->filename = directoryEntries->d_name;

                if((file = ::open(directoryEntries->d_name, O_RDONLY)) == -1)
                {
                    perror("Can't open file");
                    exit(1);
                }

                noError = true;
                do
                {
                    if((status = ::read(file, &currentChar, 1)) == -1)
                    {
                        noError = false;
                        perror("Read error");
                        exit(1);
                    }
                    else if(status == 0)
                        noError = false;
                    else
                        if(currentChar != 0)
                            newFile->filecontent += currentChar;
                } while (currentChar != 0 && noError);

                newFile->isAvailable = '+';
                newFile->peerA.sin_port = 0;
                newFile->peerB.sin_port = 0;
                newFile->socketA = -1;
                newFile->socketB = -1;

                std::cout << "Found new file: " << newFile->filename << std::endl;

                this->fileList.push_back(newFile);
            }
        }
        closedir(directory);
    }
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
    sockaddr_in peer;
    unsigned int len = sizeof(peer);

    if((client = ::accept(this->socket, (struct sockaddr*) &peer, &len)) == -1)
    {
        perror("Accept error.");
        exit(1);
    }

    std::cout << "Client connected: " << inet_ntoa(peer.sin_addr) << ":"
              << peer.sin_port << std::endl;

    this->clients.push_back(client);
}

void notepadServer::serveClient(int& socket)
{
    Json::Value object;
    std::string message = this->readStringFromSocket(socket);
    if(socket != -1)
    {
        std::stringstream(message) >> object;

        if(message != "")
        {
            std::cout << "Got message: " << message << std::endl;

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
}

void notepadServer::handleSendAllOpenFiles(int& socket)
{
    std::cout << "Sending data..." << std::endl;

    Json::Value array;
    Json::Value object;
    std::string output;

    for(openFile* file: fileList)
        array.append(file->filename + file->isAvailable);

    object["reason"] = "Send all files";
    if(!array.isNull())
    {
        std::cout << "Built file array." << std::endl;
        object["files"] = array;
        output = this->writer.write(object);

        sendString(socket, output.c_str());
    }
    else
    {
        object["files"] = Json::arrayValue;
        output = this->writer.write(object);

        sendString(socket, output.c_str());
    }

    std::cout << "Sent data..." << std::endl;
}

void notepadServer::handleReceiveNewFile(int& socket, Json::Value object)
{
    int file;
    std::string filename = object["filename"].asString();
    std::string filecontent = object["filecontent"].asString();

    std::cout << "Filename: " << filename << std::endl;
    std::cout << "File content: \"" << filename << "\"" << std::endl;

    sockaddr_in peerA;
    unsigned int len = sizeof(peerA);
    getpeername(socket, (sockaddr*) &peerA, &len);

    openFile* newFile = new openFile();
    newFile->filename = filename;
    newFile->filecontent = filecontent;
    newFile->isAvailable = '+';
    newFile->peerA = peerA;
    newFile->socketA = socket;
    newFile->peerB.sin_port = 0;
    newFile->socketB = -1;

    if((file = ::open(filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0755)) == -1)
    {
        perror("Open file for creation error");
        exit(1);
    }
    else
    {
        if(::write(file, filecontent.c_str(), filecontent.length()) == -1)
        {
            perror("Write new file error");
            exit(1);
        }
        close(file);
    }

    fileList.push_back(newFile);
}

void notepadServer::handleSendOpenFile(int& socket, Json::Value object)
{
    bool found = false;
    openFile* reference;
    std::string output;
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
        if(reference->isAvailable == '+' && reference->peerA.sin_port == 0 && reference->socketA == -1)
        {
            Json::Value newObject;
            newObject["reason"] = "Open file";
            newObject["filename"] = reference->filename;
            newObject["filecontent"] = reference->filecontent;

            output = this->writer.write(newObject);

            sendString(socket, output.c_str());

            sockaddr_in currentPeer;
            unsigned int lenCurrentPeer = sizeof(currentPeer);
            getpeername(socket, (sockaddr*) &currentPeer, &lenCurrentPeer);
            if(currentPeer.sin_addr.s_addr != reference->peerA.sin_addr.s_addr ||
               currentPeer.sin_port != reference->peerA.sin_port)
            {
                reference->peerB = currentPeer;
                reference->socketB = socket;
                reference->isAvailable = '-';
            }
        }
        else if(reference->isAvailable == '+' && reference->peerB.sin_port == 0 && reference->socketB == -1)
        {
            Json::Value newObject;
            newObject["reason"] = "Open file";
            newObject["filename"] = reference->filename;
            newObject["filecontent"] = reference->filecontent;

            output = this->writer.write(newObject);

            sendString(socket, output.c_str());

            sockaddr_in currentPeer;
            unsigned int lenCurrentPeer = sizeof(currentPeer);
            getpeername(socket, (sockaddr*) &currentPeer, &lenCurrentPeer);
            if(currentPeer.sin_addr.s_addr != reference->peerB.sin_addr.s_addr ||
               currentPeer.sin_port != reference->peerB.sin_port)
            {
                reference->peerA = currentPeer;
                reference->socketA = socket;
                reference->isAvailable = '-';
            }
        }
        else
        {
            Json::Value newObject;
            newObject["reason"] = "Open file";
            newObject["filename"] = "|NULL|";
            newObject["filecontent"] = "|NULL|";

            output = this->writer.write(newObject);

            sendString(socket, output.c_str());
        }
    }
    else
    {
        Json::Value newObject;
        newObject["reason"] = "Open file";
        newObject["filename"] = "|NULL|";
        newObject["filecontent"] = "|NULL|";

        output = this->writer.write(newObject);

        sendString(socket, output.c_str());
    }
}

void notepadServer::handleEditOpenFile(int &socket, Json::Value object)
{
    std::string output = this->writer.write(object);
    bool found = false;
    int file;
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

        if((file = ::open(reference->filename.c_str(), O_WRONLY | O_TRUNC, 0755)) == -1)
        {
            perror("Open file for edit error");
            exit(1);
        }
        else
        {
            if(::write(file, reference->filecontent.c_str(), reference->filecontent.length()) == -1)
            {
                perror("Write edited file error");
                exit(1);
            }
            close(file);
        }
    }
}

std::string notepadServer::readStringFromSocket(int& socket)
{
    std::string buffer = "";
    char currentChar;
    int bytesWritten;
    bool noError = true;

    sockaddr_in peer;
    unsigned int len = sizeof(peer);
    getpeername(socket, (sockaddr*)& peer, &len);

    do
    {
        if((bytesWritten = ::read(socket, &currentChar, 1)) == -1)
        {
            noError = false;
            perror("Read error.");
            exit(1);
        }
        else if(bytesWritten == 0)
        {
            noError = false;
            for(openFile* file: this->fileList)
            {
                if(file->peerA.sin_addr.s_addr == peer.sin_addr.s_addr && \
                   file->peerA.sin_port == peer.sin_port)
                {
                   file->peerA.sin_addr.s_addr = 0;
                   file->peerA.sin_port = 0;
                   file->socketA = -1;
                   file->isAvailable = '+';
                }
                if(file->peerB.sin_addr.s_addr == peer.sin_addr.s_addr && \
                   file->peerB.sin_port == peer.sin_port)
                {
                   file->peerB.sin_addr.s_addr = 0;
                   file->peerB.sin_port = 0;
                   file->socketB = -1;
                   file->isAvailable = '+';
                }
            }

            this->clients.erase(std::find(this->clients.begin(), this->clients.end(), socket), this->clients.end());
            close(socket);
            socket = -1;
            std::cout << "Client disconnected: " << inet_ntoa(peer.sin_addr) << ":"
                      << peer.sin_port << std::endl;
        }
        else
        {
            if(currentChar != 0)
                buffer += currentChar;
        }
    } while(currentChar != 0 && noError);

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
