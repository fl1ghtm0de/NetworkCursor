#include "server.h"
#include <csignal>

std::unique_ptr<Server> serverPtr;

void handleSignal(int signal) {
    if (signal == SIGINT && serverPtr) {
        std::cout << "\nShutting down server..." << std::endl;
        serverPtr->shutdown();
        exit(0);  // Exit the program cleanly
    }
}

int main()
{
    serverPtr = std::make_unique<Server>();
    std::signal(SIGINT, handleSignal);

    while (true)
        serverPtr->acceptAndReceive();
    return 0;
}