/*
*  Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#define WINDOW_SUBSYSTEM

#include "Log.hpp"
#include "Networker.hpp"
#include "Timer.hpp"
#include "Input.hpp"

#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#include <ios>

#pragma comment (lib, "hid.lib")

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT 27015

using namespace Input;

InputGroup  input;
HINSTANCE   hInstance;
HWND        hWnd;

static const WORD MAX_CONSOLE_LINES = 500;

int
recvHandler(SOCKET Socket, std::atomic<bool>& running)
{
    DBGOUT("rxcb - recvHandler - start...");
    int     recvResult = 1;
    char    recvbuf[DEFAULT_BUFLEN];
    int     recvbuflen = DEFAULT_BUFLEN;

    timer streamTimer([&]() {
        recvResult = recv(Socket, recvbuf, recvbuflen, 0);
        if (recvResult > 0) {
            DBGOUT("rxcb - bytes received: %d", recvResult);
            DBGOUT("rxcb - bytes : %s", recvbuf);
        }
        else if (recvResult == 0) {
            DBGOUT("rxcb - connection closed by server...");
        }
        else {
            DBGOUT("rxcb - recv failed with error: %d", WSAGetLastError());
            return 1;
        }
    }, [&recvResult, &running]() {
        return (recvResult > 0 && running.load());
    }, 0.0, -1);

    DBGOUT("rxcb - recvHandler - done");
    return 1;
}

int
sendHandler(SOCKET Socket, std::atomic<bool>& running)
{
    DBGOUT("txcb - sendHandler - start...");
    
    int sendResult = 1;
    
    char sendbuf[DEFAULT_BUFLEN];
    std::string data = "'{'joystate': {'joyt1x': '-0.647','joyt1y': '0.214'}}'";
    data.copy(sendbuf, DEFAULT_BUFLEN);

    timer streamTimer([Socket, &running, &sendResult, sendbuf]() {
        sendResult = send(Socket, sendbuf, (int)strlen(sendbuf), 0);
        if (sendResult == SOCKET_ERROR) {
            DBGOUT("txcb - send failed with error: %d", WSAGetLastError());
            running = false;
        }
        else if (sendResult == 0) {
            DBGOUT("txcb - connection closed by server...");
        }
        else {
            DBGOUT("rxcb - bytes sent: %d", sendResult);
        }
    }, [&sendResult, &running]() {
        return (sendResult > 0 && running.load());
    }, 0.016, -1);

    DBGOUT("txcb - sendHandler - done");
    return 1;
}

int
run()
{
    int res;
    Networker networker;

    do {
        if (res = networker.startClient("localhost", DEFAULT_PORT) != 0)
            return res;

        if (res = networker.startStreaming(&recvHandler, &sendHandler) != 0)
            return res;
    } while (true);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_CREATE:
    {
        DBGOUT("RAWINPUTDEVICE enumeration...");
        RAWINPUTDEVICE rid;

        rid.usUsagePage = 0x01;
        rid.usUsage = 0x05;
        rid.dwFlags = 0x00;
        rid.hwndTarget = hWnd;

        if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
            return -1;
    }
    return 0;

    case WM_INPUT:
    {
        PRAWINPUT pRawInput;
        UINT      bufferSize;
        HANDLE    hHeap;

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));

        hHeap = GetProcessHeap();
        pRawInput = (PRAWINPUT)HeapAlloc(hHeap, 0, bufferSize);
        if (!pRawInput)
            return 0;

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER));

        if (pRawInput->header.dwType == RIM_TYPEHID)
        {
            ReadGamePadInput(pRawInput, input.gamepad);
        }

        HeapFree(hHeap, 0, pRawInput);
    }
    return 0;

    case WM_MOUSEMOVE:
        DBGOUT("WM_MOUSEMOVE...");
        POINTS point;
        point = MAKEPOINTS(lParam);
        input.mouse.lX = point.x;
        input.mouse.lY = point.y;
        return 0;

    case WM_LBUTTONDOWN:
        return 0;

    case WM_LBUTTONUP:
        return 0;

    case WM_RBUTTONDOWN:
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        return 0;

    case WM_KEYDOWN:
        input.keyboard.bKeys[(unsigned char)wParam] = true;
        return 0;

    case WM_KEYUP:
        input.keyboard.bKeys[(unsigned char)wParam] = false;
        return 0;

        return DefWindowProc(hWnd, message, wParam, lParam);

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);

    }
}

BOOL createWindow(char* title, WNDPROC WndProc)
{
    WNDCLASS    wc;
    DWORD       dwExStyle;
    DWORD       dwStyle;
    RECT        WindowRect;

    hInstance = GetModuleHandle(NULL);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;

    RegisterClass(&wc);

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU;

    AdjustWindowRectEx(&WindowRect, dwStyle, false, dwExStyle);

    hWnd = CreateWindowEx(dwExStyle,
        __TEXT("tcpjoy"),
        title,
        dwStyle |
        WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        hInstance,
        NULL);

    RECT rcClient, rcWind;
    GetClientRect(hWnd, &rcClient);
    GetWindowRect(hWnd, &rcWind);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    return true;
}

//int __cdecl main(int argc, char **argv)
//{
//    if (!createWindow("tcpjoyclient", WndProc))
//        return 0;
//
//    int ret = run();
//
//    system("pause");
//
//    return ret;
//}

void destroyWindow()
{
    if (hWnd) {
        DestroyWindow(hWnd);
        hWnd = NULL;
    }

    UnregisterClass(__TEXT("tcpjoy"), hInstance);
    hInstance = NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int iCmdShow)
{
    AllocConsole();

    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    std::ios::sync_with_stdio();

    MSG msg;
    BOOL quit = FALSE;

    createWindow("tcpjoy", WndProc);

    auto worker = std::async([]() { run(); });
    auto ui = std::async([&]() {
        while (!quit) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    quit = TRUE;
                }
                else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else {
            }
        };
    });
    
    worker.get();
    ui.get();

    FreeConsole();
    destroyWindow();
    return msg.wParam;
}