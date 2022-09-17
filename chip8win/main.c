#include "main.h"
#include "chip8.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define _CRT_SECURE_NO_WARNINGS

// ********************************************************************************************************************
// ********************************************************************************************************************
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    // Create the full path to the sound file and save it off, so it can always be loaded as working directory changes
    // with loading of ROMs
    memset(_chip8_SoundFile, 0, sizeof(_chip8_SoundFile));
    GetCurrentDirectory(sizeof(_chip8_SoundFile), _chip8_SoundFile);

    wcscat_s(_chip8_SoundFile, sizeof(_chip8_SoundFile), TEXT("\\300Hz.wav"));

    _showRegisters = false;

    // Create window
    WNDCLASSW wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = L"Rectangle";
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpfnWndProc = handleWin32Message;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClassW(&wc);

    // Create the window.  Sizes are magic numbers, which is not optimal.  It would be nice if there was a way to
    // programmatically adjust window size either here or when drawing the screen.
    _hWnd = CreateWindowW(wc.lpszClassName, L"CHIP-8 Emulator", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100,
                          CHIP8_SCREEN_WIDTH * PIXEL_SIZE + 18, CHIP8_SCREEN_HEIGHT * PIXEL_SIZE + 60, NULL, NULL,
                          hInstance, NULL);

    chip8Init();

    HMENU CreateMenu();

    // Create the threads
    DWORD dwThreadId;
    _running = true;
    CreateThread(NULL, 0, threadGuiRefresh, NULL, 0, &dwThreadId);
    CreateThread(NULL, 0, threadChip8, NULL, 0, &dwThreadId);

    // Message loop
    bool bRet;
    MSG msg;
    while (bRet = GetMessage(&msg, NULL, 0, 0))
    {
        if (bRet == -1) // Error
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    _running = false;

    return (int)msg.wParam;
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void threadChip8() { chip8Run(); }

// ********************************************************************************************************************
// ********************************************************************************************************************
void threadGuiRefresh()
{
    // Delay a little before beginning the screen redraw cycle

    Sleep(200);
    while (_running)
    {
        Sleep(25); // ~40fps  TODO: Make this configurable?
        InvalidateRect(_hWnd, NULL, TRUE);
    }
}

// ********************************************************************************************************************
// ********************************************************************************************************************
LRESULT CALLBACK handleWin32Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Flag used to allow for a single erasing of the background instead of every single time the screen is drawn.  If
    // a single erase is not allowed, old stuff isn't erased properly when resizing.  If background erasing is allowed
    // all the time, tons of flickering will occur.
    static bool eraseBkgHandled = false;

    switch (msg)
    {
    case WM_CREATE: // Window created
    {
        handle_WM_CREATE(hWnd);
        break;
    }

    case WM_COMMAND: // Generated when a menu item is interacted with
    {
        handle_WM_COMMAND(hWnd, wParam, &eraseBkgHandled);
        break;
    }

    case WM_ERASEBKGND: // Generated before drawing on the window
    {
        // This message clears the window before painting occurs if we don't ignore it.  This will cause flickering
        // if we don't prematurely handle it.
        if (!eraseBkgHandled)
            eraseBkgHandled = true;
        else
            return 1;
        break;
    }
    case WM_SIZE: // Generated when window is resized
    {
        eraseBkgHandled = false;
        break;
    }
    case WM_PAINT:
    {
        handle_WM_PAINT(hWnd);
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        handle_WM_KEY(msg, wParam);
        break;
    }

    case WM_DESTROY:
    {
        _running = false;
        chip8Shutdown();
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void handle_WM_PAINT(HWND hWnd)
{
    HDC hDC;
    PAINTSTRUCT ps;

    hDC = BeginPaint(hWnd, &ps);

    uint32_t headerOffset = 0;
    if (_showRegisters)
    {
        headerOffset = REGISTER_DISPLAY_HEIGHT_PX;
    }

    // Draw the pixels of the screen
    HBRUSH hBrushBg = CreateSolidBrush(RGB(24, 24, 24));
    HBRUSH hBrushFg = CreateSolidBrush(RGB(128, 128, 128));
    for (int y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < CHIP8_SCREEN_WIDTH; x++)
        {
            SelectObject(hDC, hBrushFg);
            if (_chip8_Screen[x][y] == false) SelectObject(hDC, hBrushBg);

            Rectangle(hDC, x * PIXEL_SIZE, y * PIXEL_SIZE + headerOffset, x * PIXEL_SIZE + PIXEL_SIZE,
                      y * PIXEL_SIZE + PIXEL_SIZE + headerOffset);
        }
    }
    DeleteObject(hBrushBg);
    DeleteObject(hBrushFg);

    // Draw the registers as necessary
    if (_showRegisters)
    {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        SelectObject(hDC, hBrush);
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.bottom = rc.top + REGISTER_DISPLAY_HEIGHT_PX;
        rc.right = rc.left + REGISTER_DISPLAY_WIDTH_PX;
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

        // Set up the font
        HFONT hfont3 = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                  CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Courier New"));
        SelectObject(hDC, hfont3);

        // Set up the colors.  Green = hacker colors, obviously
        SetBkColor(hDC, RGB(0, 0, 0));
        SetTextColor(hDC, RGB(0, 255, 0));

        // Print the debug msg to screen
        // TODO: Come up with a more elegant way to do this?  Maybe wrap the mutex in an accessor function or have
        // a method that returns a copy of the message for us to use, to keep the mutex wrangling internal to chip8
        WaitForSingleObject(_chip8_Mutex, INFINITE);
        DrawTextA(hDC, _chip8_Msg, -1, &rc, DT_LEFT);
        ReleaseMutex(_chip8_Mutex);

        // Cleanup
        DeleteObject(hfont3);
        DeleteObject(hBrush);
    }

    EndPaint(hWnd, &ps);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void handle_WM_CREATE(HWND hWnd)
{
    // Add the menus to the window

    HMENU hMenubar = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hViewMenu = CreateMenu();

    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESET, L"&Reset");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_LOAD, L"&Load");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"&Exit");

    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_REGISTERS, L"&Show registers");

    AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
    SetMenu(hWnd, hMenubar);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void handle_WM_KEY(UINT msg, WPARAM wParam)
{
    bool value = msg == WM_KEYDOWN;
    switch (wParam)
    {
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9: _chip8_Keyboard[wParam - VK_NUMPAD0] = value; break;

    case 0x41: _chip8_Keyboard[0xA] = value; break;
    case 0x42: _chip8_Keyboard[0xB] = value; break;
    case 0x43: _chip8_Keyboard[0xC] = value; break;
    case 0x44: _chip8_Keyboard[0xD] = value; break;
    case 0x45: _chip8_Keyboard[0xE] = value; break;
    case 0x46: _chip8_Keyboard[0xF] = value; break;
    }
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void handle_WM_COMMAND(HWND hWnd, WPARAM wParam, bool* eraseBkgHandled)
{
    switch (LOWORD(wParam))
    {
    case IDM_FILE_RESET:
    {
        chip8Reset();
        break;
    }
    case IDM_FILE_LOAD:
    {
        char szFile[260];
        memset(szFile, 0, sizeof szFile);

        OPENFILENAME ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = TEXT("All\0*.*\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn) == TRUE)
        {
            SetCurrentDirectory(_startDirectory);
            chip8LoadRom(szFile);
            chip8Reset();
        }
        break;
    }
    case IDM_VIEW_REGISTERS:
    {
        // Toggle whether or not registers are shown
        _showRegisters = !_showRegisters;
        *eraseBkgHandled = false;

        // Resize window to properly display with/without registers
        RECT lpRect;
        GetWindowRect(hWnd, &lpRect);
        int32_t offset = -REGISTER_DISPLAY_HEIGHT_PX;
        if (_showRegisters) offset = REGISTER_DISPLAY_HEIGHT_PX;
        SetWindowPos(hWnd, NULL, lpRect.left, lpRect.top, lpRect.right - lpRect.left,
                     lpRect.bottom - lpRect.top + offset, NULL);
        break;
    }
    case IDM_FILE_EXIT:
    {
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;
    }
    }
}