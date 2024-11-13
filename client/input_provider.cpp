#include "input_provider.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#endif

InputProvider::InputProvider() {
    //startHoldingKeys();
}

InputProvider::~InputProvider() {
    //stopHoldingKeys();
}

void InputProvider::startHoldingKeys() {
    isRunning = true;
    holdKeysThread = std::thread(&InputProvider::holdKeys, this);
}

void InputProvider::stopHoldingKeys() {
    isRunning = false;
    if (holdKeysThread.joinable()) {
        holdKeysThread.join();
    }
}

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

void InputProvider::simulateKeyPress(int key, bool isPressed) {
    if (isPressed) {
        pressKey(key);
    }
    else {
        releaseKey(key);
    }
}

void InputProvider::simulateMouseClick(eKey key, bool isPressed) {
#ifdef _WIN32
    // Windows implementation using SendInput
    INPUT input = {};
    input.type = INPUT_MOUSE;

    if (key == KEY_LCLICK) {
        input.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }
    else if (key == KEY_RCLICK) {
        input.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    }

    SendInput(1, &input, sizeof(INPUT));

#elif __APPLE__
    // macOS implementation using CGEventCreateMouseEvent
    CGEventType eventType = isPressed ?
        (key == KEY_LCLICK ? kCGEventLeftMouseDown : kCGEventRightMouseDown) :
        (key == KEY_LCLICK ? kCGEventLeftMouseUp : kCGEventRightMouseUp);

    // Get the current mouse position
    CGPoint currentPos = CGEventGetLocation(CGEventCreate(NULL));

    // Create and post the event (down or up based on isPressed)
    CGEventRef mouseEvent = CGEventCreateMouseEvent(
        NULL,
        eventType,
        currentPos,
        (key == KEY_LCLICK) ? kCGMouseButtonLeft : kCGMouseButtonRight
    );
    CGEventPost(kCGHIDEventTap, mouseEvent);
    CFRelease(mouseEvent);

#elif __linux__
    // Linux implementation using XTestFakeButtonEvent (X11)
    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        std::cerr << "Unable to open X display" << std::endl;
        return;
    }

    int button = (key == KEY_LCLICK) ? 1 : 3; // Button 1 = Left Click, Button 3 = Right Click

    // Simulate mouse button press or release based on isPressed
    XTestFakeButtonEvent(display, button, isPressed ? True : False, CurrentTime);

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

void InputProvider::pressKey(int key) {
#ifdef _WIN32
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    SendInput(1, &input, sizeof(INPUT));
#elif __APPLE__
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)key, true);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
#elif __linux__
    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        std::cerr << "Unable to open X display\n";
        return;
    }
    KeyCode keycode = XKeysymToKeycode(display, key);
    XTestFakeKeyEvent(display, keycode, True, CurrentTime);
    XFlush(display);
    XCloseDisplay(display);
#endif
}

void InputProvider::releaseKey(int key) {
#ifdef _WIN32
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
#elif __APPLE__
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)key, false);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
#elif __linux__
    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        std::cerr << "Unable to open X display\n";
        return;
    }
    KeyCode keycode = XKeysymToKeycode(display, key);
    XTestFakeKeyEvent(display, keycode, False, CurrentTime);
    XFlush(display);
    XCloseDisplay(display);
#endif
}

void InputProvider::holdKeys() {
    std::set<int> previousKeys;

    while (isRunning) {  // Use isRunning to control the loop
        {
            std::lock_guard<std::mutex> lock(keyMutex);

            // Press keys that are in pressedKeys but not in previousKeys
            for (const int& key : pressedKeys) {
                if (previousKeys.find(key) == previousKeys.end()) {
                    pressKey(key);
                }
            }

            // Release keys that are in previousKeys but not in pressedKeys
            for (const int& key : previousKeys) {
                if (pressedKeys.find(key) == pressedKeys.end()) {
                    releaseKey(key);
                }
            }

            // Update previousKeys to reflect the current state of pressedKeys
            previousKeys = pressedKeys;
        }

        // Add a short delay to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Ensure all keys are released when stopping
    for (const int& key : previousKeys) {
        releaseKey(key);
    }
}