// task.h
#ifndef TASK_H
#define TASK_H

#include <string>
#include <nlohmann/json.hpp>

enum class TaskStatus { TODO, IN_PROGRESS, DONE };

struct Task {
    int id;
    std::string title;
    std::string description;
    TaskStatus status;

    nlohmann::json to_json() const;
    static Task from_json(const nlohmann::json& j);
    static std::string status_to_string(TaskStatus s);
    static TaskStatus string_to_status(const std::string& s);
};

#endif