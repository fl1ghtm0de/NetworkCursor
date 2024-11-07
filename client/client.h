#ifndef CLIENT_H
#define CLIENT_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using SOCKET_TYPE = SOCKET;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
using SOCKET_TYPE = int;
#endif

#include <string>
#include <thread>
#include <atomic>

#include "input_provider.h"

class Client {
public:
    Client(const std::string& serverAddress, int port);
    ~Client();

    bool connectToServer(int screenDirection);
    bool sendPacket(void* packet, int size);

    void startListening();
    void stopListening();

    InputProvider inputProvider;

private:
#ifdef _WIN32
    WSADATA wsaData;
#endif
    SOCKET_TYPE clientSocket;
    sockaddr_in serverAddr;

    std::thread listenerThread;
    std::atomic<bool> listening;

    std::string identifier;

    int screenWidth;
    int screenHeight;
};

#endif // CLIENT_H
