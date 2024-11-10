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

void InputProvider::simulateKeyPress(int key)
#ifdef _WIN32
{
    // KEYDOWN event
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;  // Virtual-key code
    input.ki.dwFlags = 0;   // KEYEVENTF_KEYDOWN
    input.ki.time = 0;
    input.ki.wScan = 0;     // Hardware scan code
    input.ki.dwExtraInfo = 0;

    SendInput(1, &input, sizeof(INPUT));

    // KEYUP event
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}
#elif __APPLE__
{
    // Create key down event
    CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)key, true);
    CGEventPost(kCGHIDEventTap, keyDownEvent);

    // Create key up event
    CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)key, false);
    CGEventPost(kCGHIDEventTap, keyUpEvent);

    // Release events
    CFRelease(keyDownEvent);
    CFRelease(keyUpEvent);
}
#elif __linux__

#endif

void InputProvider::simulateMouseClick(eKey key)
{
#ifdef _WIN32
    // Windows implementation using SendInput
    INPUT input[2] = {};

    if (key == KEY_LCLICK) {
        // Left mouse button down
        input[0].type = INPUT_MOUSE;
        input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        // Left mouse button up
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
}
    else if (key == KEY_RCLICK) {
        // Right mouse button down
        input[0].type = INPUT_MOUSE;
        input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

        // Right mouse button up
        input[1].type = INPUT_MOUSE;
        input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    }

    // Send the input events
    SendInput(2, input, sizeof(INPUT));

#elif __APPLE__
    // macOS implementation using CGEventCreateMouseEvent
    CGEventType downEvent = (key == KEY_LCLICK) ? kCGEventLeftMouseDown : kCGEventRightMouseDown;
    CGEventType upEvent = (key == KEY_LCLICK) ? kCGEventLeftMouseUp : kCGEventRightMouseUp;

    // Get the current mouse position
    CGPoint currentPos = CGEventGetLocation(CGEventCreate(NULL));

    // Create and post the down event
    CGEventRef clickDown = CGEventCreateMouseEvent(NULL, downEvent, currentPos,
        (key == KEY_LCLICK) ? kCGMouseButtonLeft : kCGMouseButtonRight);
    CGEventPost(kCGHIDEventTap, clickDown);
    CFRelease(clickDown);

    // Create and post the up event
    CGEventRef clickUp = CGEventCreateMouseEvent(NULL, upEvent, currentPos,
        (key == KEY_LCLICK) ? kCGMouseButtonLeft : kCGMouseButtonRight);
    CGEventPost(kCGHIDEventTap, clickUp);
    CFRelease(clickUp);

#elif __linux__
    // Linux implementation using XTestFakeButtonEvent (X11)
    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        std::cerr << "Unable to open X display" << std::endl;
        return;
    }

    int button = (key == KEY_LCLICK) ? 1 : 3; // Button 1 = Left Click, Button 3 = Right Click

    // Simulate mouse button down
    XTestFakeButtonEvent(display, button, True, CurrentTime);

    // Simulate mouse button up
    XTestFakeButtonEvent(display, button, False, CurrentTime);

    XFlush(display);
    XCloseDisplay(display);

#endif
}

int InputProvider::getPlatformKeyCode(eKey key) {
    for (const auto& pair :
#ifdef _WIN32
            windowsKeyMap
#elif __APPLE__
            macKeyMap
#elif __linux__
            linuxKeyMap
#endif
            ) {
                if (pair.second == key)
                    return pair.first;
            }

    return -1;
}
