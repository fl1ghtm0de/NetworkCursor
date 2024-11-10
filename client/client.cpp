#include "client.h"
#include <iostream>
#include <cstring> // For memcpy
#include "common/packet.h"
#include "common/defines.h"

#ifdef _WIN32
    #include <winsock2.h>
    #define closeSocket closesocket
#else
    #include <unistd.h>
    #define closeSocket close
#endif

Client::Client(const std::string& serverAddress, int port) {
    inputProvider = InputProvider();
    inputProvider.getScreenDimensions(screenWidth, screenHeight);

#ifdef _WIN32
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return;
    }
#endif

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);
}

Client::~Client() {
    stopListening();
    closeSocket(clientSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    std::cout << "Client resources cleaned up." << std::endl;
}

bool Client::connectToServer(int screenDirection) {
    if (screenDirection >= SCREEN_END) {
        std::cout << "enter value from 0 to 3; abort" << std::endl;
        return false;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed." << std::endl;
        closeSocket(clientSocket);
        return false;
    }

    std::cout << "Connected to the server." << std::endl;

    SPacketAddClient packet;
    packet.header = HEADER_ADD_CLIENT;
    packet.direction = screenDirection;
    packet.screenHeight = screenHeight;
    packet.screenWidth = screenWidth;
    memcpy(packet.identifier, identifier.c_str(), sizeof(identifier));
    sendPacket(&packet, sizeof(packet));

    SPacketResponse responsePacket;
    int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&responsePacket), sizeof(responsePacket), 0);
    if (bytesReceived > 0) {
        if (responsePacket.header == HEADER_SUCCESS_RESPONSE && responsePacket.status) {
            std::cout << "Successfully connected and received acknowledgment from server." << std::endl;
            this->identifier = identifier;
            startListening();
            return true;
        }
        else if (responsePacket.header == HEADER_SUCCESS_RESPONSE && !responsePacket.status) {
            std::cout << "Identifier " << packet.identifier << " already exists, abort" << std::endl;
            return false;
        }
        else {
            std::cerr << "Received unexpected response from server." << std::endl;
        }
    }
    else if (bytesReceived == 0) {
        std::cerr << "Server closed the connection unexpectedly." << std::endl;
    }
    else {
        #ifdef _WIN32
        std::cerr << "Failed to receive response: " << WSAGetLastError() << std::endl;
        #else
        std::cerr << "Failed to receive response: " << strerror(errno) << std::endl;
        #endif
    }

    return false;
}

bool Client::sendPacket(void* packet, int size) {
    int sendResult = send(clientSocket, static_cast<char*>(packet), size, 0);
    if (sendResult == SOCKET_ERROR) {
        #ifdef _WIN32
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        #else
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
        #endif
        return false;
    }
    //std::cout << "Message sent to the server." << std::endl;
    return true;
}

void Client::startListening() {
    listening = true;
    listenerThread = std::thread([this]() {
        while (listening) {
            char buffer[1024];
            int32_t header = -1;
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                std::memcpy(&header, buffer, sizeof(int32_t));
                switch (header) {
                    case HEADER_MOUSE_MOVE: {
                        SPacketMouseMove packet;
                        std::memcpy(&packet, buffer, sizeof(SPacketMouseMove));
                        inputProvider.moveByOffset(packet.xDelta, packet.yDelta);

                        int x;
                        int y;

                        inputProvider.getMousePosition(x, y);
                        SPacketMouseMoveResponse responsePacket = { HEADER_MOUSE_MOVE_RESPONSE, x, y };
                        sendPacket(&responsePacket, sizeof(responsePacket));
                        break;
                    }

                    case HEADER_KEYBOARD_INPUT: {
                        SPacketKeyboardInput packet;
                        std::memcpy(&packet, buffer, sizeof(SPacketKeyboardInput));
                        if (packet.key == eKey::KEY_LCLICK || packet.key == eKey::KEY_RCLICK) {
                            inputProvider.simulateMouseClick(packet.key);
                        }
                        else {
                            int mappedKey = inputProvider.getPlatformKeyCode(packet.key);
                            std::cout << "received keyboard input | key: " << packet.key << " mapped key: " << mappedKey << std::endl;
                            if (mappedKey >= 0) {
                                inputProvider.simulateKeyPress(mappedKey, packet.isPressed);
                            }
                        }
                        break;
                    }

                    default: {
                        std::cout << "received unknown header: " << header << std::endl;
                        break;
                    }
                }
            }
            else if (bytesReceived == 0) {
                std::cout << "Server closed the connection." << std::endl;
                listening = false; // Stop listening if server disconnects
            }
            else {
#ifdef _WIN32
                std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
#else
                std::cerr << "Receive failed: " << strerror(errno) << std::endl;
#endif
                listening = false; // Stop listening on error
            }
        }
        });
}

void Client::stopListening() {
    listening = false;       // Set the flag to stop the loop
    if (listenerThread.joinable()) {
        listenerThread.join(); // Wait for the listener thread to finish
    }
}