#pragma once
#include <cstdint>
#include <string>
#include <map>
#pragma pack(push, 1)  // Ensure no padding within structs

#define PORT 56569

#ifdef _WIN32
using SOCKET_TYPE = SOCKET;
#else
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
using SOCKET_TYPE = int;
#endif

struct SMonitor {
    int width;
    int height;
    int direction;

    SOCKET_TYPE clientSocket;

    SMonitor() : width(0), height(0), direction(0), clientSocket(INVALID_SOCKET) {}

    SMonitor(int width, int height, int direction, SOCKET_TYPE clientSocket)
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