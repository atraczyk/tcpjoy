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

#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

class timer
{
public:
    timer() : runCount_(0) {}

    timer(  std::function<void()> const&& func,
            std::function<bool()> const&& pred,
            double delay,
            int run_count = 0)
        : func_(std::move(func))
        , pred_(std::move(pred))
        , delay_(std::chrono::duration<double>(delay))
        , runCount_(0)
    {
        if (run_count > 0 || run_count == -1)
            run(run_count);
    };

    virtual void start( std::function<void()> const&& func,
                        std::function<bool()> const&& pred,
                        double delay,
                        int run_count) {
        func_ = func;
        pred_ = pred;
        delay_ = std::chrono::duration<double>(delay);
        run(run_count);
    }

    virtual void start(int run_count) {
        if (run_count > 0 || run_count == -1)
            run(run_count);
    }

    void stop() {
        isRunning_ = false;
    }

private:
    void run(int run_count) {
        isRunning_ = true;
        std::thread t([this, rc = std::move(run_count)]() mutable {
            while (isRunning_ && pred_()) {
                std::this_thread::sleep_for(delay_);
                if (func_) {
                    func_();
                    ++runCount_;
                }
                if (rc == 0) {
                    isRunning_ = false;
                }
                else {
                    rc = rc > 0 ? rc - 1 : rc;
                }
            }
        });
        { t.join(); }
    }

    std::atomic_int runCount_;
    std::atomic<bool> isRunning_;
    std::chrono::duration<double> delay_;
    std::function<void()> func_;
    std::function<bool()> pred_;

};
