// main.cpp
#include "client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "common/defines.h"  // Include this if it defines your IP address and port

int main() {
    std::string serverAddress = "192.168.10.46";  // or IP from defines.h
    int port = PORT; // Assuming PORT is defined in defines.h

    Client client(serverAddress, port);

    std::cout << "Enter screen alignment:\n0: right\n1: left\n2: top\n3: bottom" << std::endl;
    int direction;
    std::cin >> direction;

    if (!client.connectToServer(direction)) {
        std::cerr << "Failed to connect to the server." << std::endl;
        return 1;
    }

    while (true)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    return 0;
}