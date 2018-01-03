#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include "../include/network.h"

std::string readStringFromSocket(int& socket)
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

void sendString(int socket, const char* string)
{
    if(::write(socket, string, strlen(string) + 1) == -1)
    {
        perror("Write error.");
        exit(1);
    }
}

void sendEnd(int socket)
{
    if(::write(socket, "\0", 1) == -1)
    {
        perror("Write error.");
        exit(1);
    }
}
