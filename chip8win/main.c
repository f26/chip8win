#include "main.h"
#include "chip8.h"
#include "resource.h"

#include <stdio.h>
#include <time.h>

#define _CRT_SECURE_NO_WARNINGS

// ********************************************************************************************************************
// ********************************************************************************************************************
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                    _In_ int nShowCmd)
{
    _redrawScreen = true;
    memset(_toastMsg, 0, sizeof(_toastMsg));
    QueryPerformanceCounter(&_toastMsgTick);

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
                          CHIP8_SCREEN_WIDTH * DEFAULT_PIXEL_SIZE + 18, CHIP8_SCREEN_HEIGHT * DEFAULT_PIXEL_SIZE + 60,
                          NULL, NULL, hInstance, NULL);

    chip8InitSound(hInstance, IDR_WAVE1);
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

    case WM_MENUSELECT:
    {
        HMENU hmenu = (HMENU)lParam;
        if (hmenu == NULL) break;
        char buffer[100] = {0};
        GetMenuStringA(hmenu, (wParam & 0xFF), buffer, sizeof(buffer), MF_BYPOSITION);
        if (strcmp(buffer, "&Help") == 0)
        {
            MessageBoxA(hWnd,
                        "Emulator input:\n   Numpad 0-9: [0-9]\n   A-F: [A-F]\n\nEmulator configuration:\n   "
                        "Increase emulation speed: [NUMPAD +]\n   Decrease emulation speed: [NUMPAD -]\n   Single-step "
                        "instruction: [SPACEBAR]\n   Exit single-step mode: [ENTER]",
                        "Help I'm stuck in an emulator!", MB_OK);
        }

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
        {
            eraseBkgHandled = true;
            _redrawScreen = true;
        }
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
    case WM_NCPAINT: // Does this go here?
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
    case WM_MOVE:
    {
        // Redraw while moving to eliminate artifacts from having a portion of the window hidden and then made visible
        // TODO: Redraw in a smarter way to prevent having to redraw entire screen?
        _redrawScreen = true;
        break;
    }
        // Various messages captured during debug of selective screen redrawing.  Every single one of these was observed
        // as the window as interacted with.  Left here in case they are useful in the future.

        /*
        case WM_MOUSEMOVE:
        case WM_MOUSEHOVER:
        case WM_MOUSEACTIVATE:
        case WM_NCHITTEST:
        case WM_SETCURSOR:
        case WM_NCMOUSEMOVE:
        case WM_NCMOUSELEAVE:
        case WM_WINDOWPOSCHANGING:
        case 0x93: // WM_UAHINITMENU
        case 0x91: // WM_UAHDRAWMENU
        case 0x92: // WM_UAHDRAWMENUITEM
        case 0x94: // WM_UAHMEASUREMENUITEM
        case WM_NCLBUTTONDOWN:
        case WM_ACTIVATEAPP:
        case WM_NCACTIVATE:
        case WM_ACTIVATE:
        case WM_IME_SETCONTEXT:
        case WM_IME_NOTIFY:
        case WM_SETFOCUS:
        case WM_SYSCOMMAND:
        case WM_GETICON:
        case WM_GETMINMAXINFO:
        case WM_NCCREATE:
        case WM_NCCALCSIZE:
        case WM_WINDOWPOSCHANGED:

        case WM_SHOWWINDOW:
        case WM_DWMNCRENDERINGCHANGED:
        case WM_KILLFOCUS:
        case WM_CAPTURECHANGED:
        case WM_ENTERSIZEMOVE:
        case WM_CANCELMODE:
        case WM_MOVING:
        case WM_EXITSIZEMOVE:
        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_SYSKEYDOWN:
        case WM_SIZING:
        case WM_ENTERMENULOOP:
        case WM_INITMENU:
        case WM_MENUSELECT:
        case WM_INITMENUPOPUP:
        case WM_EXITMENULOOP:
        case WM_ENTERIDLE:
        case WM_UNINITMENUPOPUP:
        case WM_ENABLE:
        {
            break;
        }*/
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

    // Get a copy of the screen.  Static copy is used to compare to the previous frame so we can be smart about
    // drawing rectangles and not have to draw the entire screen every time.
    static bool prevScreen[CHIP8_SCREEN_WIDTH][CHIP8_SCREEN_HEIGHT] = {0};
    bool screen[CHIP8_SCREEN_WIDTH][CHIP8_SCREEN_HEIGHT];
    chip8GetScreen(&screen);

    // Recalculate pixel size depending on window dimensions so that the screen always takes up as big a region as it
    // can while maintaining the proper aspect ratio.  If screen is too small, a minimum is enforced.
    int pxSize = DEFAULT_PIXEL_SIZE;
    RECT rect;
    GetClientRect(WindowFromDC(hDC), &rect);
    double aspectRatio = (rect.right * 1.0) / (rect.bottom - headerOffset);
    if (aspectRatio <= 2.0)
    {
        // Aspect ratio < 2 means width is smaller than twice height, so width is limiting factor
        // NOTE: If exactly 2, both calculations will give same result, doesn't matter which branch we take
        pxSize = (rect.right * 1.0) / CHIP8_SCREEN_WIDTH;
    }
    else
    {
        // Aspect ratio > 2 means width is larger than twice height, so height is limiting aspect
        pxSize = ((rect.bottom - headerOffset) * 1.0) / CHIP8_SCREEN_HEIGHT;
    }
    if (pxSize < MIN_PIXEL_SIZE) pxSize = MIN_PIXEL_SIZE;

    for (int y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < CHIP8_SCREEN_WIDTH; x++)
        {
            SelectObject(hDC, hBrushFg);
            if (screen[x][y] == false) SelectObject(hDC, hBrushBg);

            if (screen[x][y] != prevScreen[x][y] || _redrawScreen)
            {
                Rectangle(hDC, x * pxSize, y * pxSize + headerOffset, x * pxSize + pxSize,
                          y * pxSize + pxSize + headerOffset);
            }
        }
    }
    memcpy(prevScreen, screen, sizeof(screen));
    _redrawScreen = false;
    DeleteObject(hBrushBg);
    DeleteObject(hBrushFg);

    // Draw the registers if necessary
    if (_showRegisters)
    {
        // Create an off-screen buffer to double buffer the register display.  This reduces flickering at high GUI
        // refresh rates. http://www.winprog.org/tutorial/bitmaps.html
        HDC hdcMem = CreateCompatibleDC(hDC);
        HBITMAP hbmMem = CreateCompatibleBitmap(hDC, REGISTER_DISPLAY_WIDTH_PX, REGISTER_DISPLAY_HEIGHT_PX);
        HANDLE hOld = SelectObject(hdcMem, hbmMem);

        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        SelectObject(hdcMem, hBrush);
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.bottom = rc.top + REGISTER_DISPLAY_HEIGHT_PX;
        rc.right = rc.left + REGISTER_DISPLAY_WIDTH_PX;
        Rectangle(hdcMem, rc.left, rc.top, rc.right, rc.bottom);

        // Set up the font
        HFONT hFont = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                 CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Courier New"));
        SelectObject(hdcMem, hFont);

        // Set up the colors.  Green = hacker colors, obviously
        SetBkColor(hdcMem, RGB(0, 0, 0));
        SetTextColor(hdcMem, RGB(0, 255, 0));

        // Print the debug msg to screen
        // TODO: Come up with a more elegant way to do this?  Maybe wrap the mutex in an accessor function or have
        // a method that returns a copy of the message for us to use, to keep the mutex wrangling internal to chip8
        WaitForSingleObject(_chip8_Mutex, INFINITE);
        DrawTextA(hdcMem, _chip8_Msg, -1, &rc, DT_LEFT);
        ReleaseMutex(_chip8_Mutex);

        // Cleanup
        DeleteObject(hFont);
        DeleteObject(hBrush);

        // Toast messages are only shown if registers are visible due to flickering issues
        // TODO: Figure out a smarter way to draw toasts to prevent flicker
        uint64_t currentTime;
        QueryPerformanceFrequency(&currentTime);
        if (getElapsedTimeSinceHighPerfTick(_toastMsgTick) < 2)
        {
            HFONT hFont = CreateFont(0, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                     CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Courier New"));
            RECT rc;
            rc.left = rc.top = 15;
            rc.right = 500;
            rc.bottom = 75;

            SetTextColor(hdcMem, RGB(255, 0, 0));
            SelectObject(hdcMem, hFont);
            DrawTextA(hdcMem, _toastMsg, -1, &rc, DT_LEFT);
            DeleteObject(hFont);
        }

        // Swap the off-screen buffer for the on-screen one, then clean up
        BitBlt(hDC, 0, 0, REGISTER_DISPLAY_WIDTH_PX, REGISTER_DISPLAY_HEIGHT_PX, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
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
    HMENU hHelpMenu = CreateMenu();

    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESET, L"&Reset");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_LOAD, L"&Load");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"&Exit");

    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_REGISTERS, L"&Show registers");

    AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
    AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
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

    case VK_RETURN:
    {
        if (_chip8_StepMode)
        {
            _chip8_StepMode = false;
            setToastMsg("Step mode disabled");
        }
        break;
    }
    case VK_SPACE:
    {
        if (!_chip8_StepMode)
        {
            setToastMsg("Step mode enabled");
        }
        _chip8_StepOnIt = true;
        _chip8_StepMode = true;
        break;
    }

    case VK_ADD:
    {

        if (_chip8_ClockSpeed == 1) // Minimum speed is 1Hz
            _chip8_ClockSpeed = 100;
        else if (_chip8_ClockSpeed < 1000) // Below 1000, increase by 100
            _chip8_ClockSpeed += 100;
        else if (_chip8_ClockSpeed < 50000) // Above 1000, increase by 100 to a max of 50000
            _chip8_ClockSpeed += 1000;

        setToastMsg("Emulation speed: %i Hz", _chip8_ClockSpeed);
        break;
    }
    case VK_SUBTRACT:
    {
        if (_chip8_ClockSpeed > 1000) // Above 1000, decrease by 1000
            _chip8_ClockSpeed -= 1000;
        else if (_chip8_ClockSpeed > 100) // Below 1000, decrease by 100
            _chip8_ClockSpeed -= 100;
        else if (_chip8_ClockSpeed == 100) // Minimum speed is 1 Hz
            _chip8_ClockSpeed = 1;

        setToastMsg("Emulation speed: %i Hz", _chip8_ClockSpeed);
        break;
    }
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
        WCHAR szFile[260];
        memset(szFile, 0, sizeof(szFile));

        OPENFILENAME ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFile = (LPWSTR)szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = TEXT("All\0*.*\0");
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn) == TRUE)
        {
            SetCurrentDirectory((LPCWSTR)_startDirectory);
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
        _redrawScreen = true;

        // Resize window to properly display with/without registers
        RECT lpRect;
        GetWindowRect(hWnd, &lpRect);
        int32_t offset = -REGISTER_DISPLAY_HEIGHT_PX;
        if (_showRegisters) offset = REGISTER_DISPLAY_HEIGHT_PX;
        SetWindowPos(hWnd, NULL, lpRect.left, lpRect.top, lpRect.right - lpRect.left,
                     lpRect.bottom - lpRect.top + offset, SWP_NOMOVE);
        break;
    }
    case IDM_FILE_EXIT:
    {
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;
    }
    }
}

// ********************************************************************************************************************
// ********************************************************************************************************************
void setToastMsg(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf_s(_toastMsg, sizeof(_toastMsg), format, args);
    va_end(args);

    QueryPerformanceCounter(&_toastMsgTick);
}