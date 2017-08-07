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

#ifdef _WIN32
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#endif

#include "Log.hpp"
#include "Server.hpp"
#include "Client.hpp"

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
        DBGOUT("starting Winsock...");
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData_);
        if (res != 0) {
            DBGOUT("WSAStartup failed with error: %d", res);
            return res;
        }
        return 0;
    };

    void
    cleanup() {
        closeServer();
        DBGOUT("cleaning up Winsock...");
        WSACleanup();
    }

    // server
    int 
    startServer(PortNumber port) {
        return server_.start(port);
    };

    int
    runServer(SocketHandlerSimple&& cb) {
        auto res = handleClient(std::move(cb));
        res.get();
        return server_.closeclientSocket();
    };

    std::future<void>
    handleClient(SocketHandlerSimple&& cb) {
        return std::async([this, &cb]() {
            SOCKET ClientSocket = server_.acceptClient();
            cb(ClientSocket);
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
    startClient(const std::string& host,
                PortNumber port) {
        return client_.connectToHost(host, port);
    };

    void
    writeToHost(const std::string& data) {
        client_.write(data);
    };

    int
    startStreaming( SocketHandler&& rxcb,
                    SocketHandler&& txcb) {
        auto rx = client_.setRecvHandler(rxcb);
        auto tx = client_.setSendHandler(txcb);
        tx.get();
        auto res = client_.disconnect();
        if (res == SOCKET_ERROR) {
            DBGOUT("startStreaming - disconnect failed with error: %d", WSAGetLastError());
            return 1;
        }
        return 0;
    };

private:
    WSADATA wsaData_;

    Server server_;
    Client client_;
};

}