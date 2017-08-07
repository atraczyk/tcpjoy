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

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

#include <ciso646>

#include "Log.hpp"

class thread_pool {
public:
    thread_pool(size_t threads = 0)
        : stop_(false),
        maxThreads_(std::max<size_t>(std::thread::hardware_concurrency(), threads))
    {
        for (size_t i = 0; i < maxThreads_; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex_);
                        this->cv_.wait(lock, [this] { return this->stop_ || !this->tasks_.empty(); });
                        if (this->stop_ && this->tasks_.empty())
                            return;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F, class... Args>
    auto run(F&& f, Args&&... args)
        ->std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared< std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            // don't allow enqueueing after destruction of the pool
            if (!stop_)
                tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return res;
    }

    ~thread_pool()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            // prevent further enqueueing
            stop_ = true;
        }
        cv_.notify_all();
        for (std::thread &worker : workers_)
            worker.join();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    bool stop_;
    const size_t maxThreads_;
};

// aberaud's ThreadPool impl from Ring

class ThreadPool {
public:
    static ThreadPool& instance() {
        static ThreadPool pool;
        return pool;
    }

    ThreadPool()
        : maxThreads_(std::max<size_t>(std::thread::hardware_concurrency(), 4))
    {
        threads_.reserve(maxThreads_);
    }

    ~ThreadPool()
    {
        join();
    }

    void run(std::function<void()>&& cb) {
        std::unique_lock<std::mutex> l(lock_);

        // launch new thread if necessary
        if (not readyThreads_ && threads_.size() < maxThreads_) {
            threads_.emplace_back(new ThreadState());
            auto& t = *threads_.back();
            t.thread = std::thread([&]() {
                while (t.run) {
                    std::function<void()> task;

                    // pick task from queue
                    {
                        std::unique_lock<std::mutex> l(lock_);
                        readyThreads_++;
                        cv_.wait(l, [&]() {
                            return not t.run or not tasks_.empty();
                        });
                        readyThreads_--;
                        if (not t.run)
                            break;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    // run task
                    try {
                        if (task) {
                            task();
                        }
                    }
                    catch (const std::exception& e) {
                        DBGOUT("Exception running task: %s", e.what());
                    }
                }
            });
        }

        // push task to queue
        tasks_.emplace(std::move(cb));

        // notify thread
        l.unlock();
        cv_.notify_one();
    }

    template<class T>
    std::future<T> get(std::function<T()>&& cb) {
        auto ret = std::make_shared<std::promise<T>>();
        run(std::bind([=](std::function<T()>& mcb) mutable {
            ret->set_value(mcb());
        }, std::move(cb)));
        return ret->get_future();
    }
    template<class T>
    std::shared_ptr<std::future<T>> getShared(std::function<T()>&& cb) {
        return std::make_shared<std::future<T>>(get(std::move(cb)));
    }

    void join() {
        for (auto& t : threads_)
            t->run = false;
        cv_.notify_all();
        for (auto& t : threads_)
            t->thread.join();
        threads_.clear();
    }

private:
    struct ThreadState
    {
        std::thread thread{};
        std::atomic_bool run{ true };
    };

    std::queue<std::function<void()>> tasks_{};
    std::vector<std::unique_ptr<ThreadState>> threads_;
    unsigned readyThreads_{ 0 };
    std::mutex lock_{};
    std::condition_variable cv_{};

    const size_t maxThreads_;
};
