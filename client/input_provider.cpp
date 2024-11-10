#include "input_provider.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#endif

InputProvider::InputProvider() {}
InputProvider::~InputProvider() {}

void InputProvider::getScreenDimensions(int& width, int& height) {
#ifdef _WIN32
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
#elif __APPLE__
    CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
    width = static_cast<int>(mainMonitor.size.width);
    height = static_cast<int>(mainMonitor.size.height);
#elif __linux__
    Screen* screen = DefaultScreenOfDisplay(display);
    width = screen->width;
    height = screen->height;
#endif
}

void InputProvider::getMousePosition(int& x, int& y) {
#ifdef _WIN32
    POINT p;
    if (GetCursorPos(&p)) {
        x = p.x;
        y = p.y;
    }
#elif __APPLE__
    CGEventRef event = CGEventCreate(nullptr);
    CGPoint point = CGEventGetLocation(event);
    x = static_cast<int>(point.x);
    y = static_cast<int>(point.y);
    CFRelease(event);
#elif __linux__
    Window root_window = DefaultRootWindow(display);
    Window returned_root, returned_child;
    int root_x, root_y;
    unsigned int mask;

    XQueryPointer(display, root_window, &returned_root, &returned_child, &root_x, &root_y, &x, &y, &mask);
#endif
}

void InputProvider::moveByOffset(int offsetX, int offsetY) {
    int currentX, currentY;
    getMousePosition(currentX, currentY);

    int newX = currentX + offsetX;
    int newY = currentY + offsetY;

    setMousePosition(newX, newY);
}

void InputProvider::setMousePosition(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);
#elif __APPLE__
    CGWarpMouseCursorPosition(CGPointMake(x, y));
#elif __linux__
    Window root_window = DefaultRootWindow(display);
    XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
    XFlush(display); // Ensure the command is sent
#endif
}

#ifdef _WIN32
void InputProvider::keyPress(WORD key) {
    INPUT inputDown;
    inputDown.type = INPUT_KEYBOARD;
    inputDown.ki.wVk = key;    // Virtual-key code
    inputDown.ki.wScan = 0;               // Hardware scan code (usually not needed)
    inputDown.ki.dwFlags = 0;             // Key down event
    inputDown.ki.time = 0;
    inputDown.ki.dwExtraInfo = 0;

    // Prepare an INPUT structure for key up
    INPUT inputUp = inputDown;
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP; // Key up event

    // Send both events in sequence
    INPUT inputs[2] = { inputDown, inputUp };
    SendInput(2, inputs, sizeof(INPUT));
}
#elif __APPLE__
void InputProvider::keyPress(char key) {
    CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(nullptr, key, true);  // Key down
    CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(nullptr, key, false);    // Key up

    if (keyDownEvent && keyUpEvent) {
        CGEventPost(kCGHIDEventTap, keyDownEvent);  // Send key down event
        CGEventPost(kCGHIDEventTap, keyUpEvent);    // Send key up event
    }

    if (keyDownEvent) CFRelease(keyDownEvent);
    if (keyUpEvent) CFRelease(keyUpEvent);
}
#endif

int InputProvider::getPlatformKeyCode(eKey key, eOS os) {
    switch (os) {
        case WIN_OS:
            return windowsKeyMap[key];
        case MAC_OS:
            return macKeyMap[key];
        case LINUX_OS:
            return linuxKeyMap[key];
        default:
            return -1;  // Fehler: OS nicht unterstützt
    }
}
