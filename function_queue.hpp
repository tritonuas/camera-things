#ifndef __FUNCTION_QUEUE_H__
#define __FUNCTION_QUEUE_H__

#include <deque>
#include <functional>
#include <iostream>


static std::deque<std::function<void()>> function_queue;
static std::mutex function_queue_mutex;

class functionQueue {

    //TODO: maybe keep track of number of things sent
    public:


        template <typename Func, typename... Args>
        void push_back_function(Func&& func, Args&&... args) {

            function_queue_mutex.lock();
            function_queue.push_back([=]() { func(args...); });
            function_queue_mutex.unlock();
        }

        template <typename Func, typename... Args>
        void push_front_function(Func&& func, Args&&... args) {

            function_queue_mutex.lock();
            function_queue.push_back([=]() { func(args...); });
            function_queue_mutex.unlock();

        }

        bool run() {
            std::function<void()> tempFunc;    
            if (!function_queue.empty()) {
                function_queue_mutex.lock();
                tempFunc = std::move(function_queue.front());
                function_queue_mutex.unlock();
                tempFunc();
            }
            return function_queue.empty();
        }


};
#endif
