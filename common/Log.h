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

#include <iostream>
#include <iomanip>
#include <array>
#include <chrono>
#include <mutex>

#include <stdarg.h>

#undef min

static std::mutex logMutex;

void
printLog(std::ostream& s, char const *m, std::array<char, 8192>& buf, int len) {
    std::lock_guard<std::mutex> lck(logMutex);

    // write timestamp
    using namespace std::chrono;
    using log_precision = microseconds;
    constexpr auto den = log_precision::period::den;
    auto num = duration_cast<log_precision>(steady_clock::now().time_since_epoch()).count();
    s << "[" << std::setfill('0') << std::setw(6) << num / den << "."
        << std::setfill('0') << std::setw(6) << num % den << "]" << " ";

    // write log
    s.write(buf.data(), std::min((size_t)len, buf.size()));
    if ((size_t)len >= buf.size())
        s << "[[TRUNCATED]]";
    s << std::endl;
}

void
consoleLog(char const *m, ...) {
    std::array<char, 8192> buffer;
    va_list args;
    va_start(args, m);
    int ret = vsnprintf(buffer.data(), buffer.size(), m, args);
    va_end(args);

    if (ret < 0)
        return;

    printLog(std::cout, m, buffer, ret);
}

#ifndef DBGOUT
#ifndef _WIN32
#define DBGOUT(m, ...) consoleLog(m, ## __VA_ARGS__)
//#define DBGOUT(format, ...) fprintf (stdout, format, ## __VA_ARGS__)
#else
#define DBGOUT(m, ...) consoleLog(m, __VA_ARGS__)
#endif
#endif
