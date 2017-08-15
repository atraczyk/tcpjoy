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

#include "cstdlib"

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

int
main(void)
{
    int ret = run();
    
    system("pause");

    return ret;
}
