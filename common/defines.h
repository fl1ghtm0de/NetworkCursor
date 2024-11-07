#pragma once
#include <cstdint>
#include <string>
#include <map>
#pragma pack(push, 1)  // Ensure no padding within structs

#define PORT 55555

struct SMonitor {
    int width;
    int height;
    int direction;
    std::map<int, std::shared_ptr<SMonitor>> neighbors;

    SOCKET clientSocket;

    SMonitor() : width(0), height(0), direction(0), clientSocket(INVALID_SOCKET) {}

    SMonitor(int width, int height, int direction, SOCKET clientSocket)
        : width(width), height(height), direction(direction), clientSocket(clientSocket) {}
};

enum eScreenDirections {
    SCREEN_RIGHT,
    SCREEN_LEFT,
    SCREEN_TOP,
    SCREEN_BOTTOM,
    SCREEN_END
};

#pragma pack(pop)