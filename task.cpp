#include "task.h"
#include <sstream>
#include <iostream>

std::string Task::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"id\":" << id << ",";
    oss << "\"title\":\"" << title << "\",";
    oss << "\"description\":\"" << description << "\",";
    oss << "\"status\":\"" << status_to_string(status) << "\"";
    oss << "}";
    return oss.str();
}

Task Task::from_json(const std::string& json_str) {
    Task task;

    // Простейший парсинг JSON
    size_t pos = 0;

    // Ищем id
    pos = json_str.find("\"id\":");
    if (pos != std::string::npos) {
        size_t start = json_str.find_first_of("0123456789", pos);
        if (start != std::string::npos) {
            size_t end = json_str.find_first_not_of("0123456789", start);
            task.id = std::stoi(json_str.substr(start, end - start));
        }
    }

    // Ищем title
    pos = json_str.find("\"title\":\"");
    if (pos != std::string::npos) {
        size_t start = pos + 9; // длина "\"title\":\""
        size_t end = json_str.find("\"", start);
        if (end != std::string::npos) {
            task.title = json_str.substr(start, end - start);
        }
    }

    // Ищем description
    pos = json_str.find("\"description\":\"");
    if (pos != std::string::npos) {
        size_t start = pos + 15; // длина "\"description\":\""
        size_t end = json_str.find("\"", start);
        if (end != std::string::npos) {
            task.description = json_str.substr(start, end - start);
        }
    }

    // Ищем status
    pos = json_str.find("\"status\":\"");
    if (pos != std::string::npos) {
        size_t start = pos + 10; // длина "\"status\":\""
        size_t end = json_str.find("\"", start);
        if (end != std::string::npos) {
            std::string status_str = json_str.substr(start, end - start);
            task.status = string_to_status(status_str);
        }
    }

    return task;
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