#include "keyMappings.h"

// Definiere die Windows KeyMap
std::map<int, eKey> windowsKeyMap = {
    // Alphabet
    {65, KEY_A}, {66, KEY_B}, {67, KEY_C}, {68, KEY_D},
    {69, KEY_E}, {70, KEY_F}, {71, KEY_G}, {72, KEY_H},
    {73, KEY_I}, {74, KEY_J}, {75, KEY_K}, {76, KEY_L},
    {77, KEY_M}, {78, KEY_N}, {79, KEY_O}, {80, KEY_P},
    {81, KEY_Q}, {82, KEY_R}, {83, KEY_S}, {84, KEY_T},
    {85, KEY_U}, {86, KEY_V}, {87, KEY_W}, {88, KEY_X},
    {89, KEY_Y}, {90, KEY_Z},

    // Numbers
    {48, KEY_0}, {49, KEY_1}, {50, KEY_2}, {51, KEY_3},
    {52, KEY_4}, {53, KEY_5}, {54, KEY_6}, {55, KEY_7},
    {56, KEY_8}, {57, KEY_9},

    // Function Keys
    {112, KEY_F1}, {113, KEY_F2}, {114, KEY_F3}, {115, KEY_F4},
    {116, KEY_F5}, {117, KEY_F6}, {118, KEY_F7}, {119, KEY_F8},
    {120, KEY_F9}, {121, KEY_F10}, {122, KEY_F11}, {123, KEY_F12},

    // Control Keys
    {13, KEY_ENTER}, {32, KEY_SPACE}, {9, KEY_TAB},
    {8, KEY_BACKSPACE}, {27, KEY_ESCAPE}, {160, KEY_LSHIFT}, {161, KEY_RSHIFT},
    {162, KEY_LCONTROL}, {163, KEY_RCONTROL}, {164, KEY_LALT}, {165, KEY_RALT},
    {20, KEY_CAPSLOCK}, {91, KEY_LWIN}, {92, KEY_RWIN}, {93, KEY_MENU},

    // Numpad Keys
    {96, KEY_NUMPAD0}, {97, KEY_NUMPAD1}, {98, KEY_NUMPAD2}, {99, KEY_NUMPAD3},
    {100, KEY_NUMPAD4}, {101, KEY_NUMPAD5}, {102, KEY_NUMPAD6}, {103, KEY_NUMPAD7},
    {104, KEY_NUMPAD8}, {105, KEY_NUMPAD9},
    {106, KEY_MULTIPLY}, {107, KEY_ADD}, {109, KEY_SUBTRACT},
    {110, KEY_DECIMAL}, {111, KEY_DIVIDE}, {144, KEY_NUMLOCK},

    // Arrow Keys
    {37, KEY_LEFT}, {39, KEY_RIGHT}, {38, KEY_UP}, {40, KEY_DOWN}
};

std::map<int, eKey> macKeyMap = {
    // Alphabet
    {0, KEY_A}, {11, KEY_B}, {8, KEY_C}, {2, KEY_D},
    {14, KEY_E}, {3, KEY_F}, {5, KEY_G}, {4, KEY_H},
    {34, KEY_I}, {38, KEY_J}, {40, KEY_K}, {37, KEY_L},
    {46, KEY_M}, {45, KEY_N}, {31, KEY_O}, {35, KEY_P},
    {12, KEY_Q}, {15, KEY_R}, {1, KEY_S}, {17, KEY_T},
    {32, KEY_U}, {9, KEY_V}, {13, KEY_W}, {7, KEY_X},
    {16, KEY_Y}, {6, KEY_Z},

    // Numbers
    {29, KEY_0}, {18, KEY_1}, {19, KEY_2}, {20, KEY_3},
    {21, KEY_4}, {23, KEY_5}, {22, KEY_6}, {26, KEY_7},
    {28, KEY_8}, {25, KEY_9},

    // Function Keys
    {122, KEY_F1}, {120, KEY_F2}, {99, KEY_F3}, {118, KEY_F4},
    {96, KEY_F5}, {97, KEY_F6}, {98, KEY_F7}, {100, KEY_F8},
    {101, KEY_F9}, {109, KEY_F10}, {103, KEY_F11}, {111, KEY_F12},

    // Control Keys
    {36, KEY_ENTER}, {49, KEY_SPACE}, {48, KEY_TAB},
    {51, KEY_BACKSPACE}, {53, KEY_ESCAPE}, {56, KEY_LSHIFT}, {60, KEY_RSHIFT},
    {59, KEY_LCONTROL}, {62, KEY_RCONTROL}, {58, KEY_LALT}, {61, KEY_RALT},
    {57, KEY_CAPSLOCK}, {55, KEY_LWIN}, {54, KEY_RWIN}, {110, KEY_MENU},

    // Numpad Keys
    {81, KEY_NUMPAD0}, {75, KEY_NUMPAD1}, {67, KEY_NUMPAD2}, {89, KEY_NUMPAD3},
    {91, KEY_NUMPAD4}, {86, KEY_NUMPAD5}, {88, KEY_NUMPAD6}, {92, KEY_NUMPAD7},
    {82, KEY_NUMPAD8}, {65, KEY_NUMPAD9},
    {69, KEY_MULTIPLY}, {78, KEY_ADD}, {74, KEY_SUBTRACT},
    {65, KEY_DECIMAL}, {75, KEY_DIVIDE}, {71, KEY_NUMLOCK},

    // Arrow Keys
    {123, KEY_LEFT}, {124, KEY_RIGHT}, {126, KEY_UP}, {125, KEY_DOWN}
};

std::map<int, eKey> linuxKeyMap = {
    // Alphabet
    {38, KEY_A}, {56, KEY_B}, {54, KEY_C}, {40, KEY_D},
    {26, KEY_E}, {41, KEY_F}, {42, KEY_G}, {43, KEY_H},
    {31, KEY_I}, {44, KEY_J}, {45, KEY_K}, {46, KEY_L},
    {58, KEY_M}, {57, KEY_N}, {32, KEY_O}, {33, KEY_P},
    {24, KEY_Q}, {27, KEY_R}, {39, KEY_S}, {28, KEY_T},
    {30, KEY_U}, {55, KEY_V}, {25, KEY_W}, {53, KEY_X},
    {29, KEY_Y}, {52, KEY_Z},

    // Numbers
    {19, KEY_0}, {10, KEY_1}, {11, KEY_2}, {12, KEY_3},
    {13, KEY_4}, {14, KEY_5}, {15, KEY_6}, {16, KEY_7},
    {17, KEY_8}, {18, KEY_9},

    // Function Keys
    {67, KEY_F1}, {68, KEY_F2}, {69, KEY_F3}, {70, KEY_F4},
    {71, KEY_F5}, {72, KEY_F6}, {73, KEY_F7}, {74, KEY_F8},
    {75, KEY_F9}, {76, KEY_F10}, {95, KEY_F11}, {96, KEY_F12},

    // Control Keys
    {36, KEY_ENTER}, {65, KEY_SPACE}, {23, KEY_TAB},
    {22, KEY_BACKSPACE}, {9, KEY_ESCAPE}, {50, KEY_LSHIFT}, {62, KEY_RSHIFT},
    {37, KEY_LCONTROL}, {105, KEY_RCONTROL}, {64, KEY_LALT}, {108, KEY_RALT},
    {66, KEY_CAPSLOCK}, {133, KEY_LWIN}, {134, KEY_RWIN}, {135, KEY_MENU},

    // Numpad Keys
    {90, KEY_NUMPAD0}, {87, KEY_NUMPAD1}, {88, KEY_NUMPAD2}, {89, KEY_NUMPAD3},
    {83, KEY_NUMPAD4}, {84, KEY_NUMPAD5}, {85, KEY_NUMPAD6}, {79, KEY_NUMPAD7},
    {80, KEY_NUMPAD8}, {81, KEY_NUMPAD9},
    {63, KEY_MULTIPLY}, {86, KEY_ADD}, {82, KEY_SUBTRACT},
    {91, KEY_DECIMAL}, {106, KEY_DIVIDE}, {77, KEY_NUMLOCK},

    // Arrow Keys
    {113, KEY_LEFT}, {114, KEY_RIGHT}, {111, KEY_UP}, {116, KEY_DOWN}
};
