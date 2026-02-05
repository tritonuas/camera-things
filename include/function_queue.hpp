#ifndef __FUNCTION_QUEUE_H__
#define __FUNCTION_QUEUE_H__

#include <deque>
#include <functional>
#include <iostream>




class functionQueue {

    static inline std::deque<std::function<void()>> function_queue;
    static inline std::mutex function_queue_mutex;
    static inline int quit_signal = 0;

    static inline int totalCount;
    int count;
    



    //TODO: maybe keep track of number of things sent
    public:
        functionQueue() {
            count = 0;
        }


        
        template <typename Func, typename... Args>
        void push_back_function(Func&& func, Args&&... args) {

            //std::cout << "function added to the quueue";
            function_queue_mutex.lock();
            function_queue.push_back([=]() { func(args...); });
            function_queue_mutex.unlock();
        }

        template <typename Func, typename... Args>
        void push_front_function(Func&& func, Args&&... args) {

            //std::cout << "function added to the quueue";
            function_queue_mutex.lock();
            function_queue.emplace_front([=]() { func(args...); });
            function_queue_mutex.unlock();

        }


        bool runOne() {
            std::function<void()> tempFunc;    
            if (function_queue.empty()) {
                //std::cout << "function queue empty";
                return true;
            }

            function_queue_mutex.lock();
            tempFunc = std::move(function_queue.front());
            function_queue.pop_front();
            function_queue_mutex.unlock();
            //std::cout << "ran one function";
            tempFunc();

            return function_queue.empty();
        }

        void runLoop() {
            quit_signal = 0;
            while (!quit_signal) {
                //std::cout << "askdj";
                runOne();
                //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        void startSendingLoop() {
            std::thread sendLoop(&functionQueue::runLoop, this);
            sendLoop.detach();
        }

        void quit() {
            //TODO: check if I need a mutex here
            quit_signal = 1;
        }

};
#endif
