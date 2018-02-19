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

#pragma once

#include <functional>
#include <string>
#include <future>

#include "Log.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Timer.hpp"

namespace Network
{

class Networker
{
public:
    Networker()
        : server_()
        , client_() {
        init();
    };
    ~Networker() {
        cleanup();
    };

    int
    init() {
#ifdef _WIN32
        WSADATA wsaData;
        DBGOUT("starting Winsock...");
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0) {
            DBGOUT("WSAStartup failed with error: %d", res);
            return res;
        }
#endif
        return 0;
    };

    void
    cleanup() {
        closeServer();
#ifdef _WIN32
        DBGOUT("cleaning up Winsock...");
        WSACleanup();
#endif
    }

    // server
    int
    startServer(PortNumber port) {
        return server_.start(port);
    };

    int
    runServer(SocketCallback&& recvcb) {
        server_.setRecvCb(recvcb);
        auto res = serverRecvHandlerAsync();
        res.get();
        return server_.closeclientSocket();
    };

    void
    setRecvCb(SocketCallback& recvcb) {
        server_.setRecvCb(recvcb);
    };

    std::future<void>
    serverRecvHandlerAsync() {
        return std::async([this]() {
            Socket ClientSocket = server_.acceptClient();
            DBGOUT("rx - recvHandler - start...");
            int     recvResult = 1;
            char    recvbuf[DEFAULT_BUFLEN];
            int     recvbuflen = DEFAULT_BUFLEN;

            while (recvResult > 0) {
                DBGOUT("rx - waiting on socket...");
                recvResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
                if (recvResult > 0) {
                    server_.getRecvCb()(ClientSocket, recvbuf, recvResult);
                }
                else if (recvResult == 0) {
                    DBGOUT("rx - connection closed by client...");
                    break;
                }
                else {
                    DBGOUT("rx - recv failed with error: %d", _socketError());
                    break;
                }
            };
            DBGOUT("rx - recvHandler - done");
        });
    };

    void
    closeServer() {
        if (server_.isRunning()) {
            DBGOUT("closing server...");
            server_.stopListening();
        }
    };

    bool
    getServerStatus() {
        return server_.isRunning();
    }

    // client
    int
    startClient(const std::string& host, PortNumber port) {
        int res = client_.connectToHost(host, port);
        if (res == 0)
            clientRecvTask_ = clientRecvHandlerAsync();
        return res;
    };

    std::future<void>
    clientRecvHandlerAsync() {
        return std::async([this]() {
            DBGOUT("rx - recvHandler - start...");
            Socket Socket = client_.getSocket();
            int     recvResult = 1;
            char    recvbuf[DEFAULT_BUFLEN];
            int     recvbuflen = DEFAULT_BUFLEN;

            while (recvResult > 0 && client_.isConnected()) {
                DBGOUT("rx - waiting on socket...");
                recvResult = recv(Socket, recvbuf, recvbuflen, 0);
                if (recvResult > 0) {
                    client_.getRecvCb()(Socket, recvbuf, recvResult);
                }
                else if (recvResult == 0) {
                    DBGOUT("rx - connection closed by client...");
                    break;
                }
                else {
                    DBGOUT("rx - recv failed with error: %d", _socketError());
                    break;
                }
            };
            DBGOUT("rx - recvHandler - done");
        });
    };

    void
    writeToHost(const std::string& data) {
        client_.write(data);
    };

    int
    startStreaming( SocketCallback&& recvcb, SocketHandler&& writer) {
        client_.setRecvCb(recvcb);
        auto txHandler = client_.setSendHandler(writer);
        txHandler.get();
        auto res = client_.disconnect();
        if (res == SOCKET_ERROR) {
            DBGOUT("startStreaming - disconnect failed with error: %d", _socketError());
            return 1;
        }
        return 0;
    };

private:
    std::future<void> clientRecvTask_;

    Server server_;
    Client client_;

};

}
