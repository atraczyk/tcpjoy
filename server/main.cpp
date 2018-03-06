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

#define DEBUG

#include "Log.hpp"
#include "Networker.hpp"

#include <cmath>
#include <cstdlib>

using namespace Network;

INPUT ip;

void
MouseSetup(INPUT *buffer)
{
    ZeroMemory(buffer, sizeof(buffer));
    buffer->type = INPUT_MOUSE;
    buffer->mi.dx = 0;
    buffer->mi.dy = 0;
    buffer->mi.mouseData = 0;
    buffer->mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;
    buffer->mi.time = 0;
    buffer->mi.dwExtraInfo = 0;
}

void
MouseMoveAbsolute(INPUT& input, int x, int y)
{
    input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN));
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(INPUT));
}

void
MouseMoveRelative(INPUT& input, int dx, int dy)
{
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;

    SendInput(1, &input, sizeof(INPUT));
}

void
MouseLeftDown(INPUT& input)
{
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void
MouseLeftUp(INPUT& input)
{
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void
MouseLeftClick(INPUT& input)
{
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    Sleep(10);

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void
SimulateKeyDown(INPUT& input, const WORD& keycode)
{
    input.ki.wVk = keycode;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));
}

void
SimulateKeyUp(INPUT& input, const WORD& keycode)
{
    input.ki.wVk = keycode;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void
SimulateKeyStroke(INPUT& input, const WORD& keycode)
{
    SimulateKeyDown(input, keycode);
    SimulateKeyUp(input, keycode);
}

void
recvCb(Socket& ClientSocket, const char* recvbuf, int recvResult)
{
    char *token = std::strtok(strdup(recvbuf), ":\0");
    if (strcmp(token, "k") == 0) {
        ip.type = INPUT_KEYBOARD;
        ip.ki.wScan = 0;
        ip.ki.time = 0;
        ip.ki.dwExtraInfo = 0;
        DBGOUT("TEXT");
        token = std::strtok(NULL, "\0");
        int len = strlen(token);
        for (int i = 0; i < len; i++) {
            auto wvk = VkKeyScanEx((char)token[i], GetKeyboardLayout(0));
            DBGOUT("Value: %c, %d", (char)token[i], wvk);
            SimulateKeyStroke(ip, wvk);
        }
    }
    else {
        MouseSetup(&ip);
        if (strcmp(token, "lu") == 0) {
            DBGOUT("MOUSEUP");
            //MouseLeftUp(ip);
        }
        else if (strcmp(token, "ld") == 0) {
            DBGOUT("MOUSEDOWN");
            //MouseLeftDown(ip);
            MouseLeftClick(ip);
        }
        else if (strcmp(token, "m") == 0) {
            int dx, dy, c = 0;
            token = std::strtok(NULL, ",\0");
            while (token != NULL && c < 2) {
                if (c == 0) {
                    dx = atoi(token);
                }
                else if (c == 1) {
                    dy = atoi(token);
                }
                c++;
                token = std::strtok(NULL, ",\0");
            }
            DBGOUT("MOUSEMOVE: (%d, %d)", dx, dy);

            MouseMoveRelative(ip, dx, dy);
        }
    }
}

int
run()
{
    int res;
    Networker nw;

    if (res = nw.startServer(DEFAULT_PORT) != 0)
        return res;

    do {
        res = nw.runServer(&recvCb) != 0;
    } while (res == 0);

    return res;
}

#include <conio.h>
#define PI 3.14159

int
main(void)
{
    //int ret = run();

    int ret = 0;
    auto running = true;
    MouseSetup(&ip);
    auto mousemove = std::thread([&running]() {
        auto a = 0.0f;
        auto amplitude = 2.0f;
        while (running) {
            auto xval = ::cos(a) * amplitude;
            auto yval = ::sin(a) * amplitude;
            a = a > 2 * PI ? 0 : a + 0.01;
            DBGOUT("mousemove a: %0.2f, xv: %0.2f - yv: %0.2f", a, xval, yval);
            MouseMoveRelative(ip, xval, yval);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        DBGOUT("done");
    });

    auto input = std::thread([&running]() {
        getch();
        running = false;
    });

    mousemove.join();
    input.join();
    system("pause");

    return ret;
}
