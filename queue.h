#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class MessageQueue {
public:
    using TaskHandler = std::function<void()>;

    void push(TaskHandler handler) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(handler);
        cv.notify_one();
    }

    void run() {
        while (!stopped) {
            TaskHandler handler;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return !queue.empty() || stopped; });

                if (stopped && queue.empty()) break;
                if (!queue.empty()) {
                    handler = queue.front();
                    queue.pop();
                }
            }
            if (handler) handler();
        }
    }

    void stop() {
        stopped = true;
        cv.notify_all();
    }

private:
    std::queue<TaskHandler> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stopped{ false };
};