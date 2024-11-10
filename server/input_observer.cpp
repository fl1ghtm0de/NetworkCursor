#include "input_observer.h"
#include "common/defines.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#endif

#ifdef _WIN32
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
#elif __APPLE__
std::map<int, std::string> keyMap = {
    {0x00, "A"}, {0x0B, "B"}, {0x08, "C"}, {0x02, "D"}, {0x0E, "E"},
    {0x03, "F"}, {0x05, "G"}, {0x04, "H"}, {0x22, "I"}, {0x26, "J"},
    {0x28, "K"}, {0x25, "L"}, {0x2E, "M"}, {0x2D, "N"}, {0x1F, "O"},
    {0x23, "P"}, {0x0C, "Q"}, {0x0F, "R"}, {0x01, "S"}, {0x13, "T"},
    {0x09, "V"}, {0x0D, "W"}, {0x07, "X"}, {0x10, "Y"}, {0x06, "Z"},
    {0x12, "1"}, {0x13, "2"}, {0x14, "3"}, {0x15, "4"}, {0x17, "5"},
    {0x16, "6"}, {0x1A, "7"}, {0x1C, "8"}, {0x19, "9"}, {0x1D, "0"},
    {0x24, "Return"}, {0x31, "Space"}, {0x30, "Tab"}, {0x33, "Delete"},
    {0x35, "Escape"}, {0x38, "Shift"}, {0x3B, "Control"}, {0x3A, "Option"}, {0x37, "Command"},
};
#elif __linux__
// missing
#endif

InputObserver* InputObserver::instance = nullptr;

InputObserver::InputObserver(const std::function<void(int, int)>& moveCallback, const std::function<void(int)>& keyPressCallback, const std::function<void(int)>& borderHitCallback)
    : onMoveCallback(moveCallback), onKeyPressCallback(keyPressCallback), onBorderHitCallback(borderHitCallback), mouseMoveThreadRunning(true) {
#ifdef _WIN32
//
#elif __APPLE__
//
#elif __linux__
    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Cannot open display\n";
        exit(1);
    }
#endif


    instance = this;
    getScreenDimensions(screenWidth, screenHeight);
    getMousePosition(currX, currY);

    start();
}

#ifdef _WIN32
void InputObserver::start() {
    mouseMoveThreadRunning = true;

    mouseMoveThread = std::thread([this]() {
        // Initialize the message-only window in this thread
        hInstance = GetModuleHandle(NULL);
        WNDCLASS wc = {};
        wc.lpfnWndProc = InputObserver::WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "MessageOnlyWindowClass";

        if (!RegisterClass(&wc)) {
            std::cerr << "Failed to register window class!" << std::endl;
            return;
        }

        // Create the message-only window
        hwnd = CreateWindowEx(
            0,
            "MessageOnlyWindowClass",
            "MessageOnlyWindow",
            0,
            0, 0, 0, 0,
            HWND_MESSAGE,
            NULL, NULL, hInstance
        );

        if (!hwnd) {
            std::cerr << "Failed to create message-only window!" << std::endl;
            return;
        }

        // Store the `this` pointer in the window user data for access in WndProc
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // Register for global raw mouse input
        RegisterGlobalRawMouseInput(hwnd);

        // Install the keyboard hook
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
        if (!keyboardHook) {
            std::cerr << "Failed to install keyboard hook!" << std::endl;
            return;
        }

        // Run the message loop in this thread
        while (mouseMoveThreadRunning) {
            update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Optional delay
        }

        // Cleanup: remove the keyboard hook and destroy the window
        UnhookWindowsHookEx(keyboardHook);
        DestroyWindow(hwnd);
        UnregisterClass("MessageOnlyWindowClass", hInstance);
        });
}

LRESULT CALLBACK InputObserver::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Use instance directly instead of retrieving from GWLP_USERDATA
    InputObserver* observer = InputObserver::instance;
    if (!observer) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_INPUT) {
        UINT dwSize;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

        LPBYTE lpb = new BYTE[dwSize];
        if (lpb == NULL) {
            return 0;
        }

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEMOUSE) {
                // Update the raw mouse deltas
                observer->sMouseData.setFromLong(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                observer->getMousePosition(observer->currX, observer->currY);

                if (observer->onBorderHitCallback) {
                    int x = observer->currX;
                    int y = observer->currY;
                    int screenWidth = observer->screenWidth;
                    int screenHeight = observer->screenHeight;

                    if (x <= 0 || x >= (screenWidth - 1) || y <= 0 || y >= (screenHeight - 1)) {
                        if (x <= 0) {
                            observer->onBorderHitCallback(SCREEN_LEFT);
                            observer->currScreen = SCREEN_LEFT;
                        }
                        else if (x >= (screenWidth - 1)) {
                            observer->onBorderHitCallback(SCREEN_RIGHT);
                            observer->currScreen = SCREEN_RIGHT;
                        }
                        else if (y <= 0) {
                            observer->onBorderHitCallback(SCREEN_TOP);
                            observer->currScreen = SCREEN_TOP;
                        }
                        else if (y >= (screenHeight - 1)) {
                            observer->onBorderHitCallback(SCREEN_BOTTOM);
                            observer->currScreen = SCREEN_BOTTOM;
                        }
                    }
                }

                if (observer->currScreen < SCREEN_END) {
                    if (observer->onMoveCallback) {
                        observer->onMoveCallback(eAxis::X_AXIS, observer->sMouseData.xDelta);
                        observer->onMoveCallback(eAxis::Y_AXIS, observer->sMouseData.yDelta);
                    }

                    observer->setMousePosition(observer->screenWidth / 2, observer->screenHeight / 2);
                }
            }
        }

        delete[] lpb;
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK InputObserver::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        int key = pKeyboard->vkCode;

        if (keyMap.find(key) != keyMap.end()) {
            if (instance->onKeyPressCallback && instance->currScreen < SCREEN_END) {
                instance->onKeyPressCallback(key);
            }
            //return 1;
        }
        else {
            std::cout << "Key pressed: VK_CODE " << key << std::endl;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void InputObserver::RegisterGlobalRawMouseInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;        // Generic desktop controls
    rid.usUsage = 0x02;            // Mouse
    rid.dwFlags = RIDEV_INPUTSINK; // Capture input even when not in focus
    rid.hwndTarget = hwnd;         // Set the message-only window handle

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        std::cerr << "Failed to register global raw input device!" << std::endl;
    }
}
#elif __APPLE__
void InputObserver::start() {
    if (isRunning) {
        std::cerr << "InputObserver is already running." << std::endl;
        return;
    }

    isRunning = true;

    mouseMoveThread = std::thread([this]() {
        IOHIDManagerRef hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (hidManager == nullptr) {
            std::cerr << "Failed to create HID Manager." << std::endl;
            return -1;
        }

        // Set up a dictionary to match mouse devices
        CFMutableDictionaryRef matchingDict = CFDictionaryCreateMutable(
            kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks
        );

        int usagePageValue = kHIDPage_GenericDesktop;
        int usageValue = kHIDUsage_GD_Mouse;

        CFNumberRef usagePage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePageValue);
        CFNumberRef usage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usageValue);

        CFDictionarySetValue(matchingDict, CFSTR(kIOHIDDeviceUsagePageKey), usagePage);
        CFDictionarySetValue(matchingDict, CFSTR(kIOHIDDeviceUsageKey), usage);

        IOHIDManagerSetDeviceMatching(hidManager, matchingDict);

        // Clean up temporary CF objects
        CFRelease(usagePage);
        CFRelease(usage);
        CFRelease(matchingDict);

        IOHIDManagerRegisterInputValueCallback(hidManager, HIDInputCallback, nullptr);

        IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

        IOReturn openStatus = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
        if (openStatus != kIOReturnSuccess) {
            std::cerr << "Failed to open HID Manager." << std::endl;
            CFRelease(hidManager);
            return -1;
        }

        std::cout << "Listening for mouse deltas..." << std::endl;
        CFRunLoopRun();

        IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
        CFRelease(hidManager);

        return 0;
    });

    keyPressThread = std::thread([this]() {
        CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);
        CFMachPortRef eventTap = CGEventTapCreate(
            kCGSessionEventTap,
            kCGHeadInsertEventTap,
            kCGEventTapOptionDefault,
            eventMask,
            keyEventCallback,
            nullptr
        );

        if (!eventTap) {
            std::cerr << "Failed to create event tap!" << std::endl;
            return;
        }

        CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

        CGEventTapEnable(eventTap, true);

        CFRunLoopRun();

        CFRelease(runLoopSource);
        CFRelease(eventTap);
    });
}

void InputObserver::HIDInputCallback(void* context, IOReturn result, void* sender, IOHIDValueRef value) {
    InputObserver* observer = InputObserver::instance;
    if (!observer) return;

    IOHIDElementRef element = IOHIDValueGetElement(value);
    uint32_t usagePage = IOHIDElementGetUsagePage(element);
    uint32_t usage = IOHIDElementGetUsage(element);

    // Check if the usage page and usage correspond to X or Y axis movement
    if (usagePage == kHIDPage_GenericDesktop) {
        int32_t movement = IOHIDValueGetIntegerValue(value);

        if (usage == kHIDUsage_GD_X) {
            // Update xDelta value
            observer->xDelta = new int(movement);
        } else if (usage == kHIDUsage_GD_Y) {
            // Update yDelta value
            observer->yDelta = new int(movement);
        }

        if (observer->xDelta && observer->yDelta) {
            int screenWidth = observer->screenWidth;
            int screenHeight = observer->screenHeight;

            observer->getMousePosition(observer->currX, observer->currY);

            if (observer->onBorderHitCallback) {
                if (observer->currX <= 0) {
                    observer->onBorderHitCallback(SCREEN_LEFT);
                }
                else if (observer->currX >= screenWidth - 1) {
                    observer->onBorderHitCallback(SCREEN_RIGHT);
                }
                else if (observer->currY <= 0) {
                    observer->onBorderHitCallback(SCREEN_TOP);
                }
                else if (observer->currY >= screenHeight - 1) {
                    observer->onBorderHitCallback(SCREEN_BOTTOM);
                }
            }

            if (observer->currScreen < SCREEN_END) {
                if (observer->onMoveCallback) {
                    observer->onMoveCallback(*(observer->xDelta), *(observer->yDelta));
                    CGWarpMouseCursorPosition(CGPointMake(screenWidth / 2, screenHeight / 2));
                }
            }

            delete observer->xDelta;
            delete observer->yDelta;
            observer->xDelta = nullptr;
            observer->yDelta = nullptr;
        }
    }
}


    CGEventRef InputObserver::keyEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
        if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
            CGKeyCode keyCode = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));

            // Print the key code
            std::cout << "Key code: " << keyCode << (type == kCGEventKeyDown ? " pressed" : " released") << std::endl;
            if (keyMap.find(keyCode) != keyMap.end()) {
                std::cout << "Blocking code: " << keyMap.at(keyCode) << std::endl;
                // return nullptr;
            }
            // Check if the key is in the blocked list
            // if (blockedKeys.find(keyCode) != blockedKeys.end()) {
            //     std::cout << "Blocking key code: " << keyCode << std::endl;
            //     return nullptr; // Return nullptr to block the event
            // }
        }

        return event; // Allow the event to proceed
    }
#endif

void InputObserver::stop() {
    mouseMoveThreadRunning = false;
    if (mouseMoveThread.joinable()) {
        mouseMoveThread.join();
    }
}

InputObserver::~InputObserver() {
#ifdef __linux__
    XCloseDisplay(display);
#endif
    mouseMoveThreadRunning = false;
}

bool InputObserver::isAtBorder()
{
    return currX == 0 || (currX - 1) >= screenWidth || currY == 0 || (currY - 1) >= screenHeight;
}

void InputObserver::update() {
#ifdef _WIN32
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

void InputObserver::moveByOffset(int offsetX, int offsetY) {
    int currentX, currentY;
    getMousePosition(currentX, currentY);

    int newX = currentX + offsetX;
    int newY = currentY + offsetY;

    setMousePosition(newX, newY);
}

void InputObserver::getScreenDimensions(int& width, int& height) {
#ifdef _WIN32
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
#elif __APPLE__
    CGRect mainMonitor = CGDisplayBounds(CGMainDisplayID());
    width = static_cast<int>(mainMonitor.size.width);
    height = static_cast<int>(mainMonitor.size.height);
    std::cout << "width: " << width << std::endl;
    std::cout << "height: " << height << std::endl;
#elif __linux__
    Screen* screen = DefaultScreenOfDisplay(display);
    width = screen->width;
    height = screen->height;
#endif
}

void InputObserver::getMousePosition(int& x, int& y) {
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

void InputObserver::setMousePosition(int x, int y) {
#ifdef _WIN32
    SetCursorPos(x, y);
#elif __APPLE__
    CGWarpMouseCursorPosition(CGPointMake(x, y));
    positionReset = true;
#elif __linux__
    Window root_window = DefaultRootWindow(display);
    XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
    XFlush(display);
#endif
}