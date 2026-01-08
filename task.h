#pragma once
#ifndef TASK_H
#define TASK_H

#include <string>

enum class TaskStatus { TODO, IN_PROGRESS, DONE };

struct Task {
    int id = 0;
    std::string title;
    std::string description;
    TaskStatus status = TaskStatus::TODO;

    std::string to_json() const;
    static Task from_json(const std::string& json_str);
    static std::string status_to_string(TaskStatus s);
    static TaskStatus string_to_status(const std::string& s);  // мнбши лернд
};

#endif