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
#include <chrono>

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

auto start = std::chrono::steady_clock::now();
std::mutex mouse_mutex;
float dx = 0, dy = 0;

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
        std::lock_guard<std::mutex> lck(mouse_mutex);
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
            int c = 0;
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

            MouseMoveRelative(ip, static_cast<int>(dx), static_cast<int>(dy));
        }
    }
}

#include <conio.h>
#define PI 3.14159f
#define EPSILON 0.5f
#define DECEL 0.8f

int
main(void)
{
    Networker nw;
    auto running = true;
    int ret = 0;

    auto network_thread = std::thread([&nw, &ret, &running]() {
        if (ret = nw.startServer(DEFAULT_PORT) != 0) {
            return;
        }
        do {
            ret = nw.runServer(&recvCb) != 0;
        } while (ret == 0 && running);
    });

    auto mouse_thread = std::thread([&running]() {
        while (running) {
            dx = fabs(dx) < EPSILON ? 0.0f : dx * DECEL;
            dy = fabs(dy) < EPSILON ? 0.0f : dy * DECEL;
            //DBGOUT("mousemove   xv: %0.2f - yv: %0.2f", dx, dy);
            MouseMoveRelative(ip, static_cast<int>(dx), static_cast<int>(dy));
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        DBGOUT("done");
    });

    auto input_thread = std::thread([&nw, &running]() {
        getch();
        nw.closeServer();
        running = false;
    });

    network_thread.join();
    mouse_thread.join();
    input_thread.join();

    system("pause");

    return ret;
}
