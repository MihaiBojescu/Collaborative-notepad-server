#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <json/json.h>

#include "../include/declarations.h"

class notepadServer
{
public:
    notepadServer();
    ~notepadServer();

private:
    void setupServerSocket();
    void getExistingFileList();
    void serve();
    void setupFileDescriptorSet();
    void setupClients();
    void serveServer();
    void serveClient(int& client);

    void handleSendAllOpenFiles(int& socket);
    void handleReceiveNewFile(int& socket, Json::Value object);
    void handleSendOpenFile(int& socket, Json::Value object);
    void handleEditOpenFile(int& socket, Json::Value object);
    void handleDisconnect(int& socket, Json::Value object);

    std::string readStringFromSocket(int& socket);
    void sendString(int& socket, std::string);

    int socket;
    int maxDescriptor;
    fd_set readFDs;
    sockaddr_in serverAddr;
    std::vector<int> clients;
    std::vector<openFile*> fileList;
    Json::FastWriter writer;
};
#endif
