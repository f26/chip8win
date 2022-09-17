#ifndef MAIN_H_
#define MAIN_H_

#include "Windows.h"
#include <stdbool.h>

#define IDM_FILE_RESET 1
#define IDM_FILE_LOAD 2
#define IDM_FILE_EXIT 3
#define IDM_VIEW_REGISTERS 4
#define MSG_WIDTH 2048
#define PIXEL_SIZE 20
#define REGISTER_DISPLAY_HEIGHT_PX 180
#define REGISTER_DISPLAY_WIDTH_PX 1200

HWND _hWnd;                // Main window, used to redraw screen
bool _running;             // Used to let GUI thread know to exit
char _startDirectory[260]; // Stores the path to the start directory
bool _showRegisters;       // Flag used to track when to draw registers

// Body of the thread that runs the emulator
void threadChip8();

// Body of the thread that performs GUI refresh
void threadGuiRefresh();

// Handler for messages
LRESULT CALLBACK handleWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Paints the window
void handle_WM_PAINT(HWND hWnd);

// Handler for the window creation message
void handle_WM_CREATE(HWND hWnd);

// Handler called whenever a key is pressed or released
void handle_WM_KEY(UINT msg, WPARAM wParam);

// Handler for messages from the menu
void handle_WM_COMMAND(HWND hWnd, WPARAM wParam, bool* eraseBkgd);

#endif
