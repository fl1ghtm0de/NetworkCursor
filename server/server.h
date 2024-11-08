#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
using SOCKET_TYPE = SOCKET;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
using SOCKET_TYPE = int;
#define CLOSE_SOCKET close
#endif

#include <string>
#include <map>
#include <mutex>
#include <memory>

#include "common/defines.h"
#include "input_observer.h"

class Server {
public:
    Server();
    ~Server();

    void acceptAndReceive();
    void handleClient(SOCKET_TYPE clientSocket);
    void sendPacketToClient(int clientDirection, void* packet, int size);
    void removeClient(int clientDirection);

    void sendMouseMovePacket(int axis, int value);
    void sendKeyPressPacket(int keyID);
    void setCurrentScreen(int direction);

    void shutdown();
private:
#ifdef _WIN32
    WSADATA wsaData;
#endif
    SOCKET_TYPE listeningSocket;
    sockaddr_in serverAddr;
    std::map<int, SMonitor> clientIDMap;
    std::mutex mapMutex;
    int currentScreen = SCREEN_END;
    InputObserver inputObserver;
};

#endif // SERVER_H
