#ifndef __FUNCTION_QUEUE_H__
#define __FUNCTION_QUEUE_H__

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include <loguru/loguru.hpp>

// [TODO]: handle failed pushes, right now we just have a large capacity
namespace functionQueue {
    // [TODO] : complete guess on capacity that we need.
    constexpr size_t CAPACITY = 1024;

    inline std::condition_variable  cv;
    inline std::mutex               function_queue_mutex;
    inline std::atomic<bool>        quit_signal = false;

    // circular buffer
    inline std::array<std::function<void()>, CAPACITY> function_queue;
    inline size_t size = 0;
    inline size_t head = 0;

    template <typename Func, typename... Args>
    void push_back_function(Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(function_queue_mutex);
        if (size == CAPACITY) {
            LOG_F(WARNING, "Failed to push to function queue.");
            return;
        }

        function_queue[(head + size) % CAPACITY] = [=]() { func(args...); };
        size++;

        cv.notify_one();
    }

    template <typename Func, typename... Args>
    void push_front_function(Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(function_queue_mutex);
        if (size == CAPACITY) {
            LOG_F(WARNING, "Failed to push to function queue.");
            return;
        }

        head = (head + CAPACITY - 1) % CAPACITY;
        function_queue[head] = [=]() { func(args...); };
        size++;

        cv.notify_one();
    }


    inline bool runOne() {
        std::function<void()> tempFunc;    
        {
            std::unique_lock<std::mutex> lock(function_queue_mutex);
            cv.wait(lock, [] { return size > 0 || quit_signal; });
                //std::cout << "function queue empty";
            if (size == 0) {
                return false;
            }

            tempFunc = std::move(function_queue[head]);
            head = (head + 1) % CAPACITY;
            size--;
        }

        tempFunc();
        return true;
    }

    inline void runLoop() {
        while (!quit_signal) {
            runOne();
        }
    }

    inline void startSendingLoop() {
        std::thread sendLoop(&functionQueue::runLoop);
        sendLoop.detach();
    }

    inline void quit() {
        quit_signal = 1;
        cv.notify_all();
    }
}  // namespace functionQueue
#endif // __FUNCTION_QUEUE_H__
