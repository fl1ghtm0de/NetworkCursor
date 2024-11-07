#ifndef INPUTPROVIDER_H
#define INPUTPROVIDER_H

class InputProvider {
public:
	InputProvider();
	~InputProvider();
	void getScreenDimensions(int& width, int& height);
	void getMousePosition(int& x, int& y);
	void moveByOffset(int offsetX, int offsetY);
	void setMousePosition(int x, int y);
};

#endif