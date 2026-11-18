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

// [TODO]: handle failed pushes
class functionQueue {
    // [TODO] : complete guess on capacity that we need.
    static constexpr size_t CAPACITY = 256;

    static inline std::condition_variable cv;
    static inline std::mutex function_queue_mutex;
    static inline std::atomic<bool> quit_signal = false;

    // circular buffer
    static inline std::array<std::function<void()>, CAPACITY> function_queue;
    static inline size_t size = 0;
    static inline size_t head = 0;

    public:
        template <typename Func, typename... Args>
        void push_back_function(Func&& func, Args&&... args) {
            std::lock_guard<std::mutex> lock(function_queue_mutex);
            if (size == CAPACITY) {
                LOG_F(ERROR, "Failed to push to function queue.");
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
                LOG_F(ERROR, "Failed to push to function queue.");
                return;
            }

            head = (head + CAPACITY - 1) % CAPACITY;
            function_queue[head] = [=]() { func(args...); };
            size++;

            cv.notify_one();
        }


        bool runOne() {
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

        void runLoop() {
            while (!quit_signal) {
                runOne();
            }
        }

        void startSendingLoop() {
            std::thread sendLoop(&functionQueue::runLoop, this);
            sendLoop.detach();
        }

        void quit() {
            quit_signal = 1;
            cv.notify_all();
        }
};
#endif
