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

#include "Log.hpp"
#include "Networker.hpp"

#include <cstdlib>

using namespace Network;

void
recvCb(Socket& ClientSocket, const char* recvbuf, int recvResult)
{
    DBGOUT("rxcb - bytes received: %d", recvResult);
    DBGOUT("rxcb - bytes : %s", recvbuf);

    auto sendResult = send(ClientSocket, recvbuf, recvResult, 0);
    if (sendResult == SOCKET_ERROR) {
        DBGOUT("rxcb - send failed with error: %d", _socketError());
    }
    DBGOUT("rxcb - bytes sent: %d", sendResult);
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

int
run()
{
    int res;
    Networker nw;

    std::thread([]() {
        auto quit = false;
        while (!quit) {
            INPUT ip;
            Sleep(1000);
            // Set up a generic keyboard event.
            ip.type = INPUT_KEYBOARD;
            ip.ki.wScan = 0; // hardware scan code for key
            ip.ki.time = 0;
            ip.ki.dwExtraInfo = 0;

            SimulateKeyStroke(ip, VK_LWIN);
            Sleep(100);
            SimulateKeyStroke(ip, 0x4E);
            Sleep(100);
            SimulateKeyStroke(ip, 0x4F);
            Sleep(100);
            SimulateKeyStroke(ip, 0x54);
            Sleep(100);
            SimulateKeyStroke(ip, VK_RETURN);

            quit = true;
        }
    }).detach();

    if (res = nw.startServer(DEFAULT_PORT) != 0)
        return res;

    do {
        res = nw.runServer(&recvCb) != 0;
    } while (res == 0);

    return res;
}

int
main(void)
{
    int ret = run();
    
    system("pause");

    return ret;
}
