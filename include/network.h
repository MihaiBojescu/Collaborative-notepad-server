#ifndef NETWORK_H
#define NETWORK_H
#include <string>

std::string readStringFromSocket(int& socket);
void sendString(int socket, const char* string);
void sendEnd(int socket);

#endif
