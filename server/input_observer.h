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


struct SMouseCoords {
    int32_t xDelta;
    int32_t yDelta;

    SMouseCoords() : xDelta(0), yDelta(0) {}

#ifdef _WIN32
    void setFromLong(LONG x, LONG y) {
        xDelta = (x < INT32_MIN) ? INT32_MIN : (x > INT32_MAX) ? INT32_MAX : static_cast<int32_t>(x);
        yDelta = (y < INT32_MIN) ? INT32_MIN : (y > INT32_MAX) ? INT32_MAX : static_cast<int32_t>(y);
    }
#elif __APPLE__
    void setFromLong(int64_t x, int64_t y) {
        xDelta = (x < std::numeric_limits<int32_t>::min()) ? std::numeric_limits<int32_t>::min()
                : (x > std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max()
                : static_cast<int32_t>(x);
        yDelta = (y < std::numeric_limits<int32_t>::min()) ? std::numeric_limits<int32_t>::min()
                : (y > std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max()
                : static_cast<int32_t>(y);
    }
#endif
};


class InputObserver {
public:
    // Constructor with callback
    InputObserver(const std::function<void(int, int)>& callback, const std::function<void(int)>& keyPressCallback, const std::function<void(int)>& borderHitcallback);
    ~InputObserver();

#ifdef _WIN32
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    void RegisterGlobalRawMouseInput(HWND hwnd);

    HINSTANCE hInstance;
    WNDCLASS wc;
    HWND hwnd;
    HHOOK keyboardHook;
#elif __APPLE__
    static void HIDCallback(void* context, IOReturn result, void* sender, IOHIDValueRef value);
    static CGEventRef keyEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
    static CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
#endif

    static InputObserver* instance;

    // Update and check for mouse movement
    void update();

    // Move the mouse by an offset
    void moveByOffset(int offsetX, int offsetY);
    void getScreenDimensions(int& width, int& height);
    void setLocked(bool state);

    bool isAtBorder();
    bool isRunning = false;
    int currScreen = SCREEN_END;

private:
    int screenWidth;
    int screenHeight;
    int currX;
    int currY;

    SMouseCoords sMouseData;
    bool mouseMoveThreadRunning;
    std::function<void(int, int)> onMoveCallback;  // Callback for mouse movement
    std::function<void(int)> onKeyPressCallback;
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