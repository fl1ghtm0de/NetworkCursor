#ifndef INPUTPROVIDER_H
#define INPUTPROVIDER_H

#ifdef _WIN32
#include "windows.h"
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif __linux__
#endif

#include "common/keyMappings.h"

#include <map>

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
	void simulateMouseClick(eKey key);

	std::map<int, int> pressedKeys;
};

#endif