// handler.cpp
#include "handler.h"

TaskManager::TaskManager(MessageQueue& mq) : message_queue(mq) {}

std::vector<Task> TaskManager::get_all_tasks() {
    std::lock_guard<std::mutex> lock(mtx);
    return tasks;
}

Task TaskManager::get_task_by_id(int id) {
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& t : tasks) {
        if (t.id == id) return t;
    }
    return Task{};
}

int TaskManager::create_task(const Task& task) {
    std::lock_guard<std::mutex> lock(mtx);
    Task new_task = task;
    new_task.id = next_id++;
    tasks.push_back(new_task);
    return new_task.id;
}

bool TaskManager::update_task(int id, const Task& task) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& t : tasks) {
        if (t.id == id) {
            t = task;
            t.id = id;
            return true;
        }
    }
    return false;
}

bool TaskManager::delete_task(int id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = std::remove_if(tasks.begin(), tasks.end(),
        [id](const Task& t) { return t.id == id; });
    if (it != tasks.end()) {
        tasks.erase(it, tasks.end());
        return true;
    }
    return false;
}