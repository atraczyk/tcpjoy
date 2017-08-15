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

#include "Log.h"
#include "Networker.hpp"
#include "Timer.hpp"

#include "SDL.h"

#include <cstdlib>
#ifndef _WIN32
#include <sys/io.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#include <ios>

#ifndef _WIN32
#define sprintf_ sprintf
#else
#define sprintf_ sprintf_s
#endif

SDL_GameController *controller = NULL;
const int JOYSTICK_DEAD_ZONE = 4000;
SDL_Event e;
int16_t lx, ly, ba, bb;

int
initializeSDL()
{
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        DBGOUT("SDL could not initialize! SDL Error: %s", SDL_GetError());
        return 1;
    }
    return 0;
}

int
getController()
{
    SDL_GameController *ctrl = NULL;

    auto njoysticks = SDL_NumJoysticks();
    DBGOUT("getControllers: %d found", njoysticks);
    for (int i = 0; i < njoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            controller = SDL_GameControllerOpen(i);
            if (controller) {
                DBGOUT("Opened gamecontroller %i", i);
                return 0;
            }
            else {
                DBGOUT("Could not open gamecontroller %i: %s", i, SDL_GetError());
                return 1;
            }
        }
    }
    return 1;
}

void
getJoyState()
{
    //pollJoystick();
    SDL_PollEvent(&e);

    lx = SDL_GameControllerGetAxis(controller,
        SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX);
    ly = SDL_GameControllerGetAxis(controller,
        SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY);
    ba = SDL_GameControllerGetButton(controller,
        SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A);
    bb = SDL_GameControllerGetButton(controller,
        SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B);

    if (std::abs(lx) < JOYSTICK_DEAD_ZONE) lx = 0;
    if (std::abs(ly) < JOYSTICK_DEAD_ZONE) ly = 0;

    DBGOUT("lx: %d", lx);
    DBGOUT("ly: %d", ly);
    DBGOUT("ba: %d", ba);
    DBGOUT("bb: %d", bb);
}

void
recvCb(Socket& ClientSocket, const char* recvbuf, int recvResult)
{
    DBGOUT("rxcb - bytes received: %d", recvResult);
    DBGOUT("rxcb - bytes : %s", recvbuf);
}

int
sendHandler(Socket Socket, std::atomic<bool>& running)
{
    DBGOUT("txh - sendHandler - start...");

    int sendResult = 1;

    char sendbuf[DEFAULT_BUFLEN];

    timer writer([Socket, &running, &sendResult, &sendbuf]() {
        getJoyState();
        sprintf_(sendbuf,
            "'{'j0':{'lx':'%d','ly':'%d','b0':'%d','b1':'%d'}}'",
            lx, ly, ba, bb);
        std::string data(sendbuf);
        if(!data.empty())
            sendResult = writeToSocket(Socket, data);
        if (sendResult == SOCKET_ERROR) {
            DBGOUT("txh - send failed with error: %d", _socketError());
            running = false;
        }
        else if (sendResult == 0) {
            DBGOUT("txh - connection closed by server...");
        }
        else {
            //DBGOUT("txh - bytes sent: %d", sendResult);
        }
    }, [&sendResult, &running]() {
        return (sendResult > 0 && running.load());
    }, 0.05, -1);

    DBGOUT("txh - sendHandler - done");
    return 1;
}

#define MAXTRIES 5
#define TRY_OR_DIE(x)   if (res = x != 0) {             \
                            if (tries < MAXTRIES - 1) { \
                                ++tries;                \
                                continue;               \
                            }                           \
                            return res;                 \
                        }

int
run()
{
    int res;
    Networker nw;
    int tries;

    do {
        tries = 0;
        TRY_OR_DIE(nw.startClient("localhost", DEFAULT_PORT));
        TRY_OR_DIE(nw.startStreaming(&recvCb, &sendHandler));
    } while (true);

    return 0;
}

#undef main
int
main(int argc, char **argv)
{
    initializeSDL();
    getController();

    int ret = run();

    system("pause");

    SDL_Quit();

    return ret;
}
