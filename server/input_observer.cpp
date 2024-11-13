#include "input_observer.h"
#include "common/defines.h"
#include "common/keyMappings.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#endif

InputObserver* InputObserver::instance = nullptr;

InputObserver::InputObserver(const std::function<void(int, int)>& moveCallback, const std::function<void(eKey, bool)>& keyPressCallback, const std::function<void(int)>& borderHitCallback)
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

        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
            if (!mouseHook) {
                std::cerr << "Failed to install mouse hook!" << std::endl;
                return;
            }

        // Run the message loop in this thread
        while (mouseMoveThreadRunning) {
            update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Optional delay
        }

        // Cleanup: remove the keyboard hook and destroy the window
        UnhookWindowsHookEx(keyboardHook);
        UnhookWindowsHookEx(mouseHook);
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
        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];

        if (lpb == NULL) {
            return 0;
        }

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEMOUSE) {
                // Update the raw mouse deltas
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
                        observer->onMoveCallback(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
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
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        bool isKeyPressed = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        for (const auto& pair : windowsKeyMap) {
            if (pair.first == pKeyboard->vkCode) {
                eKey foundKey = pair.second;
                if (instance->onKeyPressCallback && instance->currScreen < SCREEN_END) {
                    //std::cout << "Key event detected: VK_CODE " << pKeyboard->vkCode
                    //    << (isKeyPressed ? " down" : " up") << std::endl;
                    instance->onKeyPressCallback(foundKey, isKeyPressed); // Callback with key state
                    return 1; // Block the key event
                }
            }
        }

        std::cout << "VK_CODE: " << pKeyboard->vkCode << " not found" << std::endl;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK InputObserver::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        // Check for left mouse button events
        if (wParam == WM_LBUTTONDOWN) {
            if (instance->onKeyPressCallback && instance->currScreen < SCREEN_END) {
                instance->onKeyPressCallback(eKey::KEY_LCLICK, true); // Button down (pressed)
                return 1;
            }
        }
        else if (wParam == WM_LBUTTONUP) {
            if (instance->onKeyPressCallback && instance->currScreen < SCREEN_END) {
                instance->onKeyPressCallback(eKey::KEY_LCLICK, false); // Button up (released)
                return 1;
            }
        }

        // Check for right mouse button events
        else if (wParam == WM_RBUTTONDOWN) {
            if (instance->onKeyPressCallback && instance->currScreen < SCREEN_END) {
                instance->onKeyPressCallback(eKey::KEY_RCLICK, true); // Button down (pressed)
                return 1;
            }
        }
        else if (wParam == WM_RBUTTONUP) {
            if (instance->onKeyPressCallback && instance->currScreen < SCREEN_END) {
                instance->onKeyPressCallback(eKey::KEY_RCLICK, false); // Button up (released)
                return 1;
            }
        }
    }

    // Pass the event to the next hook in the chain
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
            if (macKeyMap.find(keyCode) != macKeyMap.end()) {
                std::cout << "Blocking code: " << macKeyMap.at(keyCode) << std::endl;
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