#include <iostream>
// #ifdef _WIN32
// #include <winsock2.h>
// #include <ws2tcpip.h>
// #endif
#include <thread>
#include <vector>
#include "server.h"
#include "common/defines.h"
#include "common/packet.h"
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

Server::Server() :
    inputObserver(
        [this](int axis, int value) {
            sendMouseMovePacket(axis, value);
        },
        [this](int keyCode) {
            sendKeyPressPacket(keyCode);
        },
        [this](int screenDirection) {
            setCurrentScreen(screenDirection);
        }
    )
{
#ifdef _WIN32
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return;
    }
#endif

    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
#ifdef _WIN32
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
#endif
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listeningSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
#ifdef _WIN32
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
#else
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(listeningSocket);
#endif
        return;
    }

    if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
#ifdef _WIN32
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
#else
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(listeningSocket);
#endif
        return;
    }

    std::cout << "Server initialized. Waiting for connections..." << std::endl;
}

Server::~Server() {
#ifdef _WIN32
    closesocket(listeningSocket);
    WSACleanup();
#else
    close(listeningSocket);
#endif
    std::cout << "Server resources cleaned up." << std::endl;
}

void Server::shutdown(){
#ifdef _WIN32
    closesocket(listeningSocket);
    WSACleanup();
#else
    close(listeningSocket);
#endif
    std::cout << "Server resources cleaned up." << std::endl;
}

void Server::handleClient(SOCKET_TYPE clientSocket) {
    char buffer[1024];
    int clientDirection = -1;

    while (true) {
        std::memset(buffer, 0, 1024);
        int32_t header = -1;
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);

        if (bytesReceived > 0) {
            std::memcpy(&header, buffer, sizeof(int32_t));
            switch (header) {
                case HEADER_ADD_CLIENT: {
                    SPacketAddClient packet;
                    std::memcpy(&packet, buffer, sizeof(SPacketAddClient));
                    //clientIdentifier = packet.identifier;  // Save identifier for disconnection cleanup
                    std::cout << "received AddClientHeader | " <<  "alignment: " << packet.direction << std::endl;

                    // Add client to clientIDMap if not already present
                    bool clientAdded = false;
                    {
                        std::lock_guard<std::mutex> lock(mapMutex);
                        if (clientIDMap.find(packet.direction) == clientIDMap.end()) {
                            clientIDMap.emplace(packet.direction, SMonitor(packet.screenWidth, packet.screenHeight, packet.direction, clientSocket));
                            clientAdded = true;
                        }
                    }

                    // Respond to the client
                    SPacketResponse successPacket = { HEADER_SUCCESS_RESPONSE, clientAdded };
                    send(clientSocket, reinterpret_cast<char*>(&successPacket), sizeof(SPacketResponse), 0);

                    if (!clientAdded) {
                        //std::cout << "Duplicate identifier. Closing connection with " << clientIdentifier << std::endl;
                        CLOSE_SOCKET(clientSocket);
                        return;
                    }
                    break;
                }

                case HEADER_MOUSE_MOVE_RESPONSE: {
                    SPacketMouseMoveResponse packet;
                    std::memcpy(&packet, buffer, sizeof(SPacketMouseMoveResponse));
                    std::cout << "received response mouse move packet: " << packet.x << " | " << packet.y << std::endl;
                    switch (currentScreen) {
                        case SCREEN_RIGHT: {
                            if (packet.x == 0) setCurrentScreen(SCREEN_END);
                            break;
                        }
                        case SCREEN_LEFT: {
                            if (packet.x >= (clientIDMap[currentScreen].width - 1)) setCurrentScreen(SCREEN_END);
                            break;
                        }
                        case SCREEN_BOTTOM: {
                            if (packet.y == 0) setCurrentScreen(SCREEN_END);
                            break;
                        }
                        case SCREEN_TOP: {
                            if (packet.y >= (clientIDMap[currentScreen].height - 1)) setCurrentScreen(SCREEN_END);
                            break;
                        }
                    }
                    break;
                }

                default : {
                    std::cout << "received unknown header: " << header << std::endl;
                    break;
                }
            }
        }
        else if (bytesReceived == 0) {
            std::cout << "Client with direction: " << clientDirection << " disconnected." << std::endl;
            break;
        }
        else {
            #ifdef _WIN32
            std::cerr << "Receive failed for client with direction: " << clientDirection << " error: " << WSAGetLastError() << std::endl;
            #else
            std::cerr << "Receive failed for client with direction: " << clientDirection << " error: " << strerror(errno) << std::endl;
            #endif
            break;
        }
    }

    // Remove client from clientIDMap on disconnection
    if (clientDirection != -1) {
        std::lock_guard<std::mutex> lock(mapMutex);
        clientIDMap.erase(clientDirection);
    }

    // Close the client socket after the loop ends
    CLOSE_SOCKET(clientSocket);
    std::cout << "Closed connection with client with direction: " << clientDirection << "." << std::endl;
    removeClient(clientDirection);
}

void Server::acceptAndReceive() {
    while (true) {
        SOCKET_TYPE clientSocket = accept(listeningSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
#ifdef _WIN32
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
#else
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
#endif
            continue;
        }

        std::cout << "Client connected!" << std::endl;

        // Launch a new thread to handle the client
        std::thread clientThread(&Server::handleClient, this, clientSocket);
        clientThread.detach(); // Detach the thread to let it run independently
    }
}

void Server::removeClient(int clientDirection) {
    std::lock_guard<std::mutex> lock(mapMutex);
    clientIDMap.erase(clientDirection);
}

void Server::sendPacketToClient(int clientDirection, void* packet, int size) {
    std::cout << "Attempting to send packet to direction: " << clientDirection << std::endl;

    SOCKET_TYPE clientSocket;
    {
        std::lock_guard<std::mutex> lock(mapMutex);
        auto it = clientIDMap.find(clientDirection);
        if (it != clientIDMap.end()) {
            clientSocket = it->second.clientSocket;
        }
        else {
            std::cerr << "Client direction: " << clientDirection << " not found." << std::endl;
            setCurrentScreen(SCREEN_END);
            return;
        }
    }

    int sendResult = send(clientSocket, reinterpret_cast<char*>(packet), size, 0);
    if (sendResult == SOCKET_ERROR) {
        #ifdef _WIN32
        std::cerr << "Failed to send packet to direction" << clientDirection << " error: " << WSAGetLastError() << std::endl;
        #else
        std::cerr << "Failed to send packet to direction" << clientDirection << " error: " << strerror(errno) << std::endl;
        #endif
        std::lock_guard<std::mutex> lock(mapMutex);
        clientIDMap.erase(clientDirection);
    }
    else {
        std::cout << "Packet sent to direction: " << clientDirection << "." << std::endl;
    }
}

void Server::setCurrentScreen(int direction) {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = clientIDMap.find(direction);
    if (it != clientIDMap.end()) {
        currentScreen = direction;
        inputObserver.currScreen = direction;
    }
    else {
        std::cerr << "Client direction: " << direction << " not found." << std::endl;
        currentScreen = SCREEN_END;
        inputObserver.currScreen = SCREEN_END;
        return;
    }
}

void Server::sendMouseMovePacket(int axis, int value) {
    SPacketMouseMove packet;
    packet.header = HEADER_MOUSE_MOVE;
    switch (axis) {
        case eAxis::X_AXIS: {
            packet.xDelta = value;
            break;
        }

        case eAxis::Y_AXIS: {
            packet.yDelta = value;
            break;
        }

        default: {
            std::cout << "invalid axis value: " << axis << std::endl;
            return;
        }
    }
    if (currentScreen < SCREEN_END) {
        sendPacketToClient(currentScreen, &packet, sizeof(packet));
    }
}

void Server::sendKeyPressPacket(int keyID) {
    SPacketKeyboardInput packet = { HEADER_KEYBOARD_INPUT, keyID };
    if (currentScreen < SCREEN_END) {
        sendPacketToClient(currentScreen, &packet, sizeof(packet));
    }
}