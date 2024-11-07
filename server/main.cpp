#include "server.h"

int main()
{
    Server s1;
    while (true)
        s1.acceptAndReceive();
    return 0;
}