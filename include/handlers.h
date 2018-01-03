#ifndef HANDLERS_H
#define HANDLERS_H
#include <vector>

#include "../include/declarations.h"

void handleSendAllOpenFiles(int& socket, std::vector<openFile*>& fileList);
void handleReceiveNewFile(int& socket, std::string message, std::vector<openFile*>& fileList);
void handleSendOpenFile(int& socket, std::string message, std::vector<openFile*>& fileList);

#endif
