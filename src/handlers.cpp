#include <stdio.h>
#include <string.h>
#include "../include/handlers.h"
#include "../include/network.h"
#include "../include/declarations.h"

void handleSendAllOpenFiles(int& socket, std::vector<openFile*>& fileList)
{
    printf("Sending data...\n");
    fflush(stdout);

    for(openFile* file: fileList)
            sendString(socket, (file->filename + file->isAvailable).c_str());
    sendEnd(socket);

    printf("Sent data...\n");
    fflush(stdout);
}

void handleReceiveNewFile(int& socket, std::string message, std::vector<openFile*>& fileList)
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

void handleSendOpenFile(int& socket, std::string message, std::vector<openFile*>& fileList)
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
