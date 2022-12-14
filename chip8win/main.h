#ifndef MAIN_H_
#define MAIN_H_

#include "Windows.h"
#include <stdbool.h>
#include <stdint.h>

#define IDM_FILE_RESET 1
#define IDM_FILE_LOAD 2
#define IDM_FILE_EXIT 3
#define IDM_VIEW_REGISTERS 4
#define MSG_WIDTH 2048
#define DEFAULT_PIXEL_SIZE 20
#define MIN_PIXEL_SIZE 5
#define REGISTER_DISPLAY_HEIGHT_PX 180
#define REGISTER_DISPLAY_WIDTH_PX 1200

HWND _hWnd;                 // Main window, used to redraw screen
bool _running;              // Used to let GUI thread know to exit
WCHAR _startDirectory[260]; // Stores the path to the start directory
bool _showRegisters;        // Flag used to track when to draw registers
char _toastMsg[100];        // Buffer to hold the toast message
uint64_t _toastMsgTick;     // The tick when the toast msg was set, from QueryPerformanceCounter()
bool _redrawScreen;         // Set when the entire CHIP-8 screen needs to be redrawn

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

// Sets the toast message that temporarily informs the user of information
void setToastMsg(const char* format, ...);

#endif
