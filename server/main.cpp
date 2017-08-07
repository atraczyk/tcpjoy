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

#include "cstdlib"

using namespace Network;

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT 27015

int
clientCallBack(SOCKET ClientSocket)
{
    DBGOUT("rxcb - start...");
    int     recvResult = 1;
    char    recvbuf[DEFAULT_BUFLEN];
    int     recvbuflen = DEFAULT_BUFLEN;
    
    while (recvResult > 0) {
        recvResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (recvResult > 0) {
            DBGOUT("rxcb - bytes received: %d", recvResult);
            DBGOUT("rxcb - bytes : %s", recvbuf);

            auto sendResult = send(ClientSocket, recvbuf, recvResult, 0);
            if (sendResult == SOCKET_ERROR) {
                DBGOUT("rxcb - send failed with error: %d", WSAGetLastError());
                return 1;
            }
            DBGOUT("rxcb - bytes sent: %d", sendResult);
        }
        else if (recvResult == 0) {
            DBGOUT("rxcb - connection closed by client...");
        }
        else {
            DBGOUT("rxcb - recv failed with error: %d", WSAGetLastError());
            return 1;
        }
    };
    return 1;
}

int
run()
{
    int res;
    Networker networker;

    if (res = networker.startServer(DEFAULT_PORT) != 0)
        return res;

    do {
        res = networker.runServer(&clientCallBack) != 0;
    } while (res == 0);

    return res;
}

int __cdecl main(void)
{
    int ret = run();
    
    system("pause");

    return ret;
}
