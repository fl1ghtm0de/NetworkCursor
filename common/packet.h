#pragma once
#include <cstdint>
#include <string>
#include <map>
#include "common/defines.h"
#pragma pack(push, 1)  // Ensure no padding within structs

struct SPacketAddClient {
    int32_t header;
    char identifier[64];
    int screenWidth;
    int screenHeight;
    int direction;

    SPacketAddClient() : header(0), screenWidth(0), screenHeight(0), direction(0) {
        std::memset(identifier, 0, sizeof(identifier));
    }
};

struct SPacketMouseMove {
    int32_t header;
    int32_t xDelta;
    int32_t yDelta;

    SPacketMouseMove() : header(-1), xDelta(0), yDelta(0) {};
};

struct SPacketMouseMoveResponse {
    int32_t header;
    int32_t x;
    int32_t y;
};

struct SPacketKeyboardInput {
    int32_t header;
    eKey key;
    eOS os;
};

struct SPacketResponse {
    int32_t header;
    bool status;
};

enum eHeaders {
    HEADER_ADD_CLIENT,
    HEADER_MOUSE_MOVE,
    HEADER_MOUSE_MOVE_RESPONSE,
    HEADER_KEYBOARD_INPUT,
    HEADER_SUCCESS_RESPONSE,
};

#pragma pack(pop)