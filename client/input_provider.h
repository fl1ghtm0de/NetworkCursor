#ifndef INPUTPROVIDER_H
#define INPUTPROVIDER_H

#ifdef _WIN32
#include "windows.h"
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#endif

#include "common/keyMappings.h"

#include <set>
#include <mutex>
#include <thread>
#include <chrono>

class InputProvider {
public:
	InputProvider();
	~InputProvider();
	void getScreenDimensions(int& width, int& height);
	void getMousePosition(int& x, int& y);
	void moveByOffset(int offsetX, int offsetY);
	void setMousePosition(int x, int y);
	int getPlatformKeyCode(eKey key);
	void simulateKeyPress(int key, bool isPressed);
	void simulateMouseClick(eKey key, bool isPressed);

private:
	void pressKey(int key);
	void releaseKey(int key);
#ifdef __APPLE__
	void createMouseEvent(CGEventType type, CGPoint position, CGMouseButton button);
#endif
	bool lmbPressed = false;
	bool rmbPressed = false;

};

#endif