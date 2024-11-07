#include "input_observer.h"
#include "common/defines.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#endif

InputObserver* InputObserver::instance = nullptr;

InputObserver::InputObserver(const std::function<void(int, int)>& moveCallback, const std::function<void(int)>& keyPressCallback, const std::function<void(int)>& borderHitCallback)
    : onMoveCallback(moveCallback), onKeyPressCallback(keyPressCallback), onBorderHitCallback(borderHitCallback), mouseMoveThreadRunning(true) {
#ifdef __linux__
    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Cannot open display\n";
        exit(1);
    }
#endif

    //getMousePosition(lastX, lastY);

    instance = this;

    getScreenDimensions(screenWidth, screenHeight);
    getMousePosition(currX, currY);

    start();
}

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

// Call this method to stop the thread and join it
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
                        observer->onMoveCallback(observer->sMouseData.xDelta, observer->sMouseData.yDelta);
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

        if (instance->keyMap.find(key) != instance->keyMap.end()) {
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

bool InputObserver::isAtBorder()
{
    return currX == 0 || (currX - 1) >= screenWidth || currY == 0 || (currY - 1) >= screenHeight;
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

void InputObserver::update() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
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
#elif __linux__
    Screen* screen = DefaultScreenOfDisplay(display);
    width = screen->width;
    height = screen->height;
#endif
}

// Platform-specific implementations for getting and setting mouse position
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
#elif __linux__
    Window root_window = DefaultRootWindow(display);
    XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
    XFlush(display); // Ensure the command is sent
#endif
}