#ifndef KEY_MAPPINGS_H
#define KEY_MAPPINGS_H

#include <map>

enum eOS {
    WIN_OS,
    MAC_OS,
    LINUX_OS
};

enum eKey {
    // Alphabet keys
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
    KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P,
    KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X,
    KEY_Y, KEY_Z,

    // Number keys
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7,
    KEY_8, KEY_9,

    // Function keys
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7,
    KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

    // Modifier keys
    KEY_LSHIFT, KEY_RSHIFT,     // Left and Right Shift
    KEY_LCONTROL, KEY_RCONTROL, // Left and Right Control
    KEY_LALT, KEY_RALT,         // Left and Right Alt
    KEY_CAPSLOCK, KEY_MENU,

    // Control and navigation keys
    KEY_ENTER, KEY_SPACE, KEY_TAB, KEY_BACKSPACE, KEY_ESCAPE,

    // Arrow keys
    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,

    // Numpad keys
    KEY_NUMPAD0, KEY_NUMPAD1, KEY_NUMPAD2, KEY_NUMPAD3, KEY_NUMPAD4,
    KEY_NUMPAD5, KEY_NUMPAD6, KEY_NUMPAD7, KEY_NUMPAD8, KEY_NUMPAD9,
    KEY_NUMLOCK, KEY_MULTIPLY, KEY_ADD, KEY_SUBTRACT, KEY_DECIMAL,
    KEY_DIVIDE,

    // Special system keys
    KEY_LWIN, KEY_RWIN,          // Left and Right Windows/Command keys

    // Mouse keys
    KEY_LCLICK, KEY_RCLICK
};


// Deklariere die KeyMaps nur als extern
extern std::map<int, eKey> windowsKeyMap;
extern std::map<int, eKey> macKeyMap;
extern std::map<int, eKey> linuxKeyMap;

#endif // KEY_MAPPINGS_H
