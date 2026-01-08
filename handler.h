#ifndef HANDLER_H
#define HANDLER_H

#include "task.h"
#include "queue.h"
#include <vector>

class TaskManager {
public:
    TaskManager(MessageQueue& mq);
    std::vector<Task> get_all_tasks();
    Task get_task_by_id(int id);
    int create_task(const Task& task);
    bool update_task(int id, const Task& task);
    bool delete_task(int id);

private:
    std::vector<Task> tasks;
    int next_id = 1;
    MessageQueue& message_queue;
    std::mutex mtx;
};

#endif