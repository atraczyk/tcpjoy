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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <error.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#else
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")
#endif

#include "Log.h"

#ifndef _WIN32
using Socket = int;
using Host = hostent*;
#define INVALID_SOCKET          (~0)
#define SOCKET_ERROR            (-1)
#else
using Socket = SOCKET;
using Host = std::string;
#endif

using PortNumber = uint16_t;

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT 27015

namespace Network
{

using SocketHandler = std::function<void(Socket, std::atomic<bool>&)>;
using SocketCallback = std::function<void(Socket&, const char*, int)>;

void
_close(Socket socket)
{
#ifndef _WIN32
    close(socket);
#else
    closesocket(socket);
#endif
}

int
_socketError()
{
#ifndef _WIN32
    return 1;
#else
    return WSAGetLastError();
#endif
}

int
writeToSocket(Socket socket, const std::string& data) {
    const char *sendbuf = data.c_str();
    auto res = send(socket, sendbuf, (int)strlen(sendbuf), 0);
    if (res == SOCKET_ERROR) {
        DBGOUT("send failed with error: %d", _socketError());
        return res;
    }
    return res;
};

class Client {
public:
    Client()
        : connectSocket_(INVALID_SOCKET)
        , connected_(false)
        , receiving_(false)
        , transmitting_(false) { };
    // todo: disable copy semantics and enable move semantics
    ~Client() {};

    int
    connectToHost(const std::string& host, PortNumber port = 0)
    {
        DBGOUT("connecting to %s...", host.c_str());
        if (port)
            portNumber_ = port;

#ifndef _WIN32
        if ((host_ = gethostbyname(host.c_str())) == NULL) {
            perror("gethostbyname:");
            DBGOUT("unable to gethostbyname: %s", host.c_str());
            return 1;
        }

        if ((connectSocket_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket:");
            DBGOUT("unable to create socket");
            return 1;
        }

        sockaddr_in serv_addr;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(DEFAULT_PORT);
        serv_addr.sin_addr = *((struct in_addr *)host_->h_addr);
        bzero(&(serv_addr.sin_zero), 8);

        if (connect(connectSocket_,
            (struct sockaddr *)&serv_addr,
            sizeof(struct sockaddr)) == -1) {
            DBGOUT("unable to connect to server");
            return 1;
        }
#else
        host_ = host;
        unsigned res = 0;

        addrinfo *addressResult = nullptr;
        addrinfo *addressAttempt = nullptr;
        addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        res = getaddrinfo(host_.c_str(),
            std::to_string(portNumber_).c_str(),
            &hints,
            &addressResult);
        if (res != 0) {
            DBGOUT("getaddrinfo failed with error: %d", res);
            return 1;
        }

        for (addressAttempt = addressResult;
            addressAttempt != NULL;
            addressAttempt = addressAttempt->ai_next) {
            connectSocket_ = socket(addressAttempt->ai_family,
                addressAttempt->ai_socktype,
                addressAttempt->ai_protocol);
            if (connectSocket_ == INVALID_SOCKET) {
                freeaddrinfo(addressResult);
                DBGOUT("socket failed with error: %ld", _socketError());
                return 1;
            }

            res = connect(connectSocket_, addressAttempt->ai_addr,
                (int)addressAttempt->ai_addrlen);
            if (res == SOCKET_ERROR) {
                _close(connected_);
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

#endif /* _WIN32 */
        connected_ = true;
        DBGOUT("connected to %s...", host);
        return 0;
    };

    int
    disconnect() {
        DBGOUT("closing connected socket...");
        return closeConnectedSocket();
    };

    int
    write(const std::string& data) {
        auto res = writeToSocket(connectSocket_, data);
        if (res == SOCKET_ERROR) {
            DBGOUT("write failed with error: %d", _socketError());
            return res;
        }
        return res;
    };

    int
    closeConnectedSocket() {
        if (isConnected()) {
            DBGOUT("shutting down connected socket...");
            connected_ = false;
#ifdef _WIN32
            int res = shutdown(connectSocket_, SD_SEND);
            if (res == SOCKET_ERROR) {
                DBGOUT("shutdown failed with error: %ld", _socketError());
                _close(connectSocket_);
                return 1;
            }
#endif
            _close(connectSocket_);
            return 0;
        }
        return 1;
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

    Socket
    getSocket() {
        return connectSocket_;
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
    Host host_;
    PortNumber portNumber_;
    Socket connectSocket_;
    std::atomic<bool> connected_;

    std::atomic<bool> receiving_;
    SocketCallback recvCb_;

    std::atomic<bool> transmitting_;
    SocketHandler sendHandler_;
    std::shared_future<void> sendFuture_;

};

}
