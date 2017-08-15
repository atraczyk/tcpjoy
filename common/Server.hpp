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

#ifndef _WIN32
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
#endif

#include "Log.h"
#include "Client.hpp"

using namespace Network;

namespace Network
{

class Server {
public:
    Server(PortNumber port = 0)
        : portNumber_(port)
        , listenSocket_(INVALID_SOCKET)
        , clientSocket_(INVALID_SOCKET)
        , connected_(false)
        , running_(false) { };
    ~Server() {
        if (isRunning())
            stopListening();
    };

    Socket
    acceptClient() {
        DBGOUT("waiting for client...");
        clientSocket_ = accept(listenSocket_, NULL, NULL);
        if (clientSocket_ == INVALID_SOCKET) {
            DBGOUT("accept failed with error: %ld", _socketError());
            closeclientSocket();
        }
        DBGOUT("client connected");
        connected_ = true;
        return clientSocket_;
    };

    int
    start(PortNumber port = 0) {
        DBGOUT("starting server...");

        if (port)
            portNumber_ = port;

        int res = 0;

        addrinfo *addressResult = nullptr;
        addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        res = getaddrinfo(  NULL,
                            std::to_string(portNumber_).c_str(),
                            &hints,
                            &addressResult);
        if (res != 0) {
            DBGOUT("getaddrinfo failed with error: %d", res);
            return 1;
        }

        listenSocket_ = socket( addressResult->ai_family,
                                addressResult->ai_socktype,
                                addressResult->ai_protocol);
        if (listenSocket_ == INVALID_SOCKET) {
            DBGOUT("socket failed with error: %ld", _socketError());
            freeaddrinfo(addressResult);
            return 1;
        }

        res = bind( listenSocket_,
                    addressResult->ai_addr,
                    (int)addressResult->ai_addrlen);
        if (res == SOCKET_ERROR) {
            DBGOUT("bind failed with error: %ld", _socketError());
            freeaddrinfo(addressResult);
            _close(listenSocket_);
            return 1;
        }

        freeaddrinfo(addressResult);

        res = listen(listenSocket_, SOMAXCONN);
        if (res == SOCKET_ERROR) {
            DBGOUT("listen failed with error: %ld", _socketError());
            _close(listenSocket_);
            return 1;
        }

        running_ = true;
        DBGOUT("server running");
        return 0;
    };

    int
    closeclientSocket() {
        if (isConnected()) {
            DBGOUT("shutting down connected socket...");
            connected_ = false;
#ifdef _WIN32
            int res = shutdown(clientSocket_, SD_SEND);
            if (res == SOCKET_ERROR) {
                DBGOUT("shutdown failed with error: %ld", _socketError());
                _close(clientSocket_);
                return 1;
            }
#endif
            _close(clientSocket_);
            return 0;
        }
        return 1;
    };

    int
    stopListening()
    {
        closeclientSocket();
        if (isRunning()) {
            running_ = false;
            DBGOUT("closing listen socket...");
            _close(listenSocket_);
            return 0;
        }
        return 1;
    };

    bool
    isRunning() {
        return running_.load();
    };

    bool
    isConnected() {
        return connected_.load();
    };

    SocketCallback&
    getRecvCb() {
        return recvCb_;
    };

    void
    setRecvCb(SocketCallback& cb) {
        recvCb_ = std::move(cb);
    };

private:
    PortNumber portNumber_;

    Socket listenSocket_;
    Socket clientSocket_;

    SocketCallback recvCb_;

    std::atomic<bool> running_;
    std::atomic<bool> connected_;

};

}
