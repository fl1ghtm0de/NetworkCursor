#ifndef INPUTOBSERVER_H
#define INPUTOBSERVER_H

#include <iostream>
#include <functional>  // For std::function
#include <thread>
#include <map>
#include <limits>
#include <cstdint>     // For int32_t, int64_t

#ifdef _WIN32
    #include <windows.h>
#elif __APPLE__
    #include <IOKit/hid/IOHIDManager.h>
    #include <ApplicationServices/ApplicationServices.h>
    #include <unordered_set>
#elif __linux__
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif

#include "common/defines.h"
#include "common/keyMappings.h"

class InputObserver {
public:
    // Constructor with callback
    InputObserver(const std::function<void(int, int)>& callback, const std::function<void(eKey)>& keyPressCallback, const std::function<void(int)>& borderHitcallback);
    ~InputObserver();

#ifdef _WIN32
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    void RegisterGlobalRawMouseInput(HWND hwnd);

    HINSTANCE hInstance;
    WNDCLASS wc;
    HWND hwnd;
    HHOOK keyboardHook;

    LONG xDelta;
    LONG yDelta;
#elif __APPLE__
    int* xDelta = nullptr;
    int* yDelta = nullptr;

    static void HIDInputCallback(void* context, IOReturn result, void* sender, IOHIDValueRef value);
    static CGEventRef keyEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
#endif

    static InputObserver* instance;

    // Update and check for mouse movement
    void update();

    // Move the mouse by an offset
    void moveByOffset(int offsetX, int offsetY);
    void getScreenDimensions(int& width, int& height);
    bool isAtBorder();

    bool isRunning = false;
    bool positionReset = false;
    int currScreen = SCREEN_END;

private:
    int screenWidth;
    int screenHeight;
    int currX;
    int currY;

    //SMouseCoords sMouseData;
    bool mouseMoveThreadRunning;
    std::function<void(int, int)> onMoveCallback;  // Callback for mouse movement
    std::function<void(eKey)> onKeyPressCallback;
    std::function<void(int)> onBorderHitCallback;

    std::thread mouseMoveThread;
    std::thread keyPressThread;
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