#ifndef INPUTOBSERVER_H
#define INPUTOBSERVER_H

#include <iostream>
#include <functional>  // For std::function
#include <thread>
#include <map>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "common/defines.h"

struct SMouseCoords {
    int32_t xDelta;
    int32_t yDelta;

    SMouseCoords() : xDelta(0), yDelta(0) {}

    void setFromLong(LONG x, LONG y) {
        xDelta = (x < INT32_MIN) ? INT32_MIN : (x > INT32_MAX) ? INT32_MAX : static_cast<int32_t>(x);
        yDelta = (y < INT32_MIN) ? INT32_MIN : (y > INT32_MAX) ? INT32_MAX : static_cast<int32_t>(y);
    }
};


class InputObserver {
public:
    // Constructor with callback
    InputObserver(const std::function<void(int, int)>& callback, const std::function<void(int)>& keyPressCallback, const std::function<void(int)>& borderHitcallback);
    ~InputObserver();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    static InputObserver* instance;

    // Update and check for mouse movement
    void update();

    // Move the mouse by an offset
    void moveByOffset(int offsetX, int offsetY);
    void getScreenDimensions(int& width, int& height);
    void setLocked(bool state);

    bool isAtBorder();

    int currScreen = SCREEN_END;

private:
    HINSTANCE hInstance;
    WNDCLASS wc;
    HWND hwnd;
    HHOOK keyboardHook;

    int screenWidth;
    int screenHeight;
    int currX;
    int currY;

    std::map<int, std::string> keyMap = {
        {0x41, "A"}, {0x42, "B"}, {0x43, "C"}, {0x44, "D"}, {0x45, "E"},
        {0x46, "F"}, {0x47, "G"}, {0x48, "H"}, {0x49, "I"}, {0x4A, "J"},
        {0x4B, "K"}, {0x4C, "L"}, {0x4D, "M"}, {0x4E, "N"}, {0x4F, "O"},
        {0x50, "P"}, {0x51, "Q"}, {0x52, "R"}, {0x53, "S"}, {0x54, "T"},
        {0x55, "U"}, {0x56, "V"}, {0x57, "W"}, {0x58, "X"}, {0x59, "Y"},
        {0x5A, "Z"}, {0x30, "0"}, {0x31, "1"}, {0x32, "2"}, {0x33, "3"},
        {0x34, "4"}, {0x35, "5"}, {0x36, "6"}, {0x37, "7"}, {0x38, "8"},
        {0x39, "9"}, {VK_RETURN, "Enter"}, {VK_SPACE, "Space"},
        {VK_TAB, "Tab"}, {VK_BACK, "Backspace"}, {VK_ESCAPE, "Escape"},
        {VK_SHIFT, "Shift"}, {VK_CONTROL, "Control"}, {VK_MENU, "Alt"}
    };

    void RegisterGlobalRawMouseInput(HWND hwnd);

    SMouseCoords sMouseData;
    bool mouseMoveThreadRunning;
    std::function<void(int, int)> onMoveCallback;  // Callback for mouse movement
    std::function<void(int)> onKeyPressCallback;
    std::function<void(int)> onBorderHitCallback;
    std::thread mouseMoveThread;
    void start();
    void stop();
#ifdef __linux__
    Display* display;
#endif

    // Platform-specific method to get the current mouse position
    void getMousePosition(int& x, int& y);

    // Platform-specific method to set the mouse position
    void setMousePosition(int x, int y);
};

#endif // INPUTOBSERVER_H