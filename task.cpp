// task.cpp
#include "task.h"

using json = nlohmann::json;

json Task::to_json() const {
    return {
        {"id", id},
        {"title", title},
        {"description", description},
        {"status", status_to_string(status)}
    };
}

Task Task::from_json(const json& j) {
    Task t;
    t.id = j.value("id", 0);
    t.title = j.value("title", "");
    t.description = j.value("description", "");
    t.status = string_to_status(j.value("status", "todo"));
    return t;
}

std::string Task::status_to_string(TaskStatus s) {
    switch (s) {
    case TaskStatus::TODO: return "todo";
    case TaskStatus::IN_PROGRESS: return "in_progress";
    case TaskStatus::DONE: return "done";
    default: return "todo";
    }
}

TaskStatus Task::string_to_status(const std::string& s) {
    if (s == "in_progress") return TaskStatus::IN_PROGRESS;
    if (s == "done") return TaskStatus::DONE;
    return TaskStatus::TODO;
}