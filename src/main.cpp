#include <signal.h>
#include "../include/server.h"

notepadServer* server;

static void sigHandler(int signal)
{
    delete server;
    exit(0);
}

int main(int argc, char** argv)
{
    struct sigaction newAction, oldAction;
    
    newAction.sa_handler = sigHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;

    sigaction(SIGINT, NULL, &oldAction);
    if(oldAction.sa_handler != SIG_IGN)
        sigaction(SIGINT, &newAction, NULL);

    server = new notepadServer();
    return 0;
}
