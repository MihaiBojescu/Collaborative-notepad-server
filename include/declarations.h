#ifndef DECLARATIONS_H
#define DECLARATIONS_H
#include <string>
#include <netinet/in.h>

struct openFile
{
    std::string filename;
    std::string filecontent;
    char isAvailable;
    sockaddr_in peerA;
    sockaddr_in peerB;
    int socketA;
    int socketB;
};

#endif
