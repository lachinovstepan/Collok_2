// queue.cpp
#include "queue.h"

void MessageQueue::push(TaskHandler handler) {
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(handler);
    cv.notify_one();
}

void MessageQueue::run() {
    while (true) {
        TaskHandler handler;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return !queue.empty() || stopped; });
            if (stopped && queue.empty()) break;
            handler = queue.front();
            queue.pop();
        }
        handler();
    }
}

void MessageQueue::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    stopped = true;
    cv.notify_all();
}