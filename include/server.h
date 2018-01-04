#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/declarations.h"

class notepadServer
{
public:
    notepadServer();
    ~notepadServer();

private:
    static notepadServer* instance;
    void setupServerSocket();
    void serve();
    void setupFileDescriptorSet();
    void setupClients();
    void serveServer();
    void serveClient(int& client);

    void handleSendAllOpenFiles(int& socket);
    void handleReceiveNewFile(int& socket, std::string message);
    void handleSendOpenFile(int& socket, std::string message);

    std::string readStringFromSocket(int& socket);
    void sendString(int& socket, std::string);
    void sendEnd(int& socket);

    int socket;
    int maxDescriptor;
    fd_set readFDs;
    sockaddr_in serverAddr;
    sockaddr_in receiveAddr;
    std::vector<int> clients;
    std::vector<openFile*> fileList;
};
#endif
