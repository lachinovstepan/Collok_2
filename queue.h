// queue.h
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>

class MessageQueue {
public:
    using TaskHandler = std::function<void()>;

    void push(TaskHandler handler);
    void run();
    void stop();

private:
    std::queue<TaskHandler> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stopped = false;
};

#endif