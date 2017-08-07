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

namespace Network
{

using PortNumber = uint16_t;
using SocketHandler = std::function<void(SOCKET,std::atomic<bool>&)>;
using SocketHandlerSimple = std::function<void(SOCKET)>;

class Client {
public:
    Client(): connected_(false) { };
    Client(PortNumber port) : connected_(false) { };
    // todo: disable copy semantics and enable move semantics
    ~Client() {};

    int
    connectToHost(std::string host, PortNumber port = 0) {
        DBGOUT("connecting to %s...", host);

        if (port)
            portNumber_ = port;

        int res = 0;

        addrinfo *addressResult = nullptr;
        addrinfo *addressAttempt = nullptr;
        addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        res = getaddrinfo(  host.c_str(),
                            std::to_string(portNumber_).c_str(),
                            &hints,
                            &addressResult);
        if (res != 0) {
            DBGOUT("getaddrinfo failed with error: %d", res);
            return 1;
        }

        for (   addressAttempt = addressResult;
                addressAttempt != NULL;
                addressAttempt = addressAttempt->ai_next) {
            connectSocket_ = socket(addressAttempt->ai_family,
                                    addressAttempt->ai_socktype,
                                    addressAttempt->ai_protocol);
            if (connectSocket_ == INVALID_SOCKET) {
                freeaddrinfo(addressResult);
                DBGOUT("socket failed with error: %ld", WSAGetLastError());
                return 1;
            }

            res = connect(  connectSocket_,
                            addressAttempt->ai_addr,
                            (int)addressAttempt->ai_addrlen);
            if (res == SOCKET_ERROR) {
                closesocket(connectSocket_);
                connectSocket_ = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(addressResult);

        if (connectSocket_ == INVALID_SOCKET) {
            DBGOUT("unable to connect to server");
            return 1;
        }

        connected_ = true;
        DBGOUT("connected to %s...", host);
        return 0;
    };

    int
    disconnect() {
        DBGOUT("closing connected socket...");
        transmitting_ = false;
        receiving_ = false;
        recvFuture_.get();
        sendFuture_.get();
        /*{
            SocketHandler empty;
            std::swap(recvHandler_, empty);
        }
        {
            SocketHandler empty;
            std::swap(sendHandler_, empty);
        }*/
        auto res = shutdown(connectSocket_, SD_SEND);
        if (res == SOCKET_ERROR) {
            DBGOUT("shutdown failed with error: %ld", WSAGetLastError());
            closeConnectedSocket();
            return 1;
        }
        closeConnectedSocket();
        return 0;
    };

    int
    write(const std::string& data) {
        const char *sendbuf = data.c_str();
        auto res = send(connectSocket_, sendbuf, (int)strlen(sendbuf), 0);
        if (res == SOCKET_ERROR) {
            DBGOUT("send failed with error: %d", WSAGetLastError());
            closeConnectedSocket();
            return 1;
        }

        DBGOUT("Bytes Sent: %ld", res);
    };

    int
    closeConnectedSocket() {
        if (isConnected()) {
            DBGOUT("shutting down connected socket...");
            connected_ = false;
            int res = shutdown(connectSocket_, SD_SEND);
            if (res == SOCKET_ERROR) {
                DBGOUT("shutdown failed with error: %ld", WSAGetLastError());
                closesocket(connectSocket_);
                return 1;
            }
            return 0;
        }
        return 1;
    };

    std::shared_future<void>
    setRecvHandler(SocketHandler& cb) {
        recvHandler_ = cb;
        DBGOUT("setRecvHandler...");
        recvFuture_ = std::async([this, cb]() {
            cb(connectSocket_, receiving_);
        });
        receiving_ = true;
        return recvFuture_;
    };

    std::shared_future<void>
    setSendHandler(SocketHandler& cb) {
        sendHandler_ = cb;
        DBGOUT("setSendHandler...");
        sendFuture_ = std::async([this, cb]() {
            cb(connectSocket_, transmitting_);
        });
        transmitting_ = true;
        return sendFuture_;
    };

    bool
    getSocket() {
        return connectSocket_;
    };

    bool
    isConnected() {
        return connected_.load();
    };

private:
    std::string host_;
    PortNumber portNumber_;

    SOCKET connectSocket_;

    std::atomic_bool connected_;
    std::atomic_bool transmitting_;
    std::atomic_bool receiving_;

    SocketHandler recvHandler_;
    SocketHandler sendHandler_;

    std::shared_future<void> recvFuture_;
    std::shared_future<void> sendFuture_;
};

}