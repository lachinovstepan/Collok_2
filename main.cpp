#include "handler.h"
#include "queue.h"
#include "task.h"
#include "httplib.h"
#include <iostream>
#include <sstream>

using namespace httplib;
using namespace std;

// Простая функция для создания JSON ошибки
string create_error(const string& message) {
    return "{\"error\":\"" + message + "\"}";
}

int main() {
    cout << "=== To-Do API Server ===\n";

    MessageQueue mq;
    TaskManager manager(mq);

    // Запускаем обработку очереди в отдельном потоке
    thread worker([&mq]() {
        mq.run();
        });

    Server svr;

    // ========== GET /tasks - все задачи ==========
    svr.Get("/tasks", [&manager](const Request& req, Response& res) {
        cout << "GET /tasks\n";
        auto tasks = manager.get_all_tasks();
        string result = "[";
        for (size_t i = 0; i < tasks.size(); i++) {
            result += tasks[i].to_json();
            if (i < tasks.size() - 1) result += ",";
        }
        result += "]";
        res.set_content(result, "application/json");
        });

    // ========== POST /tasks - создать задачу ==========
    svr.Post("/tasks", [&manager, &mq](const Request& req, Response& res) {
        cout << "POST /tasks\n";

        // Создаем копии для лямбды
        string body = req.body;

        // Добавляем в очередь
        mq.push([&manager, body, &res]() {
            Task task = Task::from_json(body);
            if (task.title.empty()) {
                res.set_content(create_error("Title is required"), "application/json");
                res.status = 400;
                return;
            }

            int id = manager.create_task(task);
            task.id = id;
            res.set_content(task.to_json(), "application/json");
            res.status = 201;
            });
        });

    // ========== GET /tasks/{id} - получить задачу ==========
    svr.Get(R"(/tasks/(\d+))", [&manager](const Request& req, Response& res) {
        int id = stoi(req.matches[1]);
        cout << "GET /tasks/" << id << "\n";

        Task task = manager.get_task_by_id(id);
        if (task.id == 0) {
            res.status = 404;
            res.set_content(create_error("Task not found"), "application/json");
            return;
        }

        res.set_content(task.to_json(), "application/json");
        });

    // ========== PUT /tasks/{id} - обновить задачу ==========
    svr.Put(R"(/tasks/(\d+))", [&manager, &mq](const Request& req, Response& res) {
        int id = stoi(req.matches[1]);
        cout << "PUT /tasks/" << id << "\n";

        string body = req.body;

        mq.push([&manager, id, body, &res]() {
            Task task = Task::from_json(body);
            task.id = id;

            if (manager.update_task(id, task)) {
                res.set_content(task.to_json(), "application/json");
            }
            else {
                res.status = 404;
                res.set_content(create_error("Task not found"), "application/json");
            }
            });
        });

    // ========== PATCH /tasks/{id} - обновить статус ==========
    svr.Patch(R"(/tasks/(\d+))", [&manager, &mq](const Request& req, Response& res) {
        int id = stoi(req.matches[1]);
        cout << "PATCH /tasks/" << id << "\n";

        string body = req.body;

        mq.push([&manager, id, body, &res]() {
            // Простой парсинг JSON для получения статуса
            size_t pos = body.find("\"status\"");
            if (pos == string::npos) {
                res.status = 400;
                res.set_content(create_error("Status field required"), "application/json");
                return;
            }

            size_t start = body.find(':', pos);
            size_t quote1 = body.find('"', start);
            size_t quote2 = body.find('"', quote1 + 1);

            if (quote1 == string::npos || quote2 == string::npos) {
                res.status = 400;
                res.set_content(create_error("Invalid JSON"), "application/json");
                return;
            }

            string status = body.substr(quote1 + 1, quote2 - quote1 - 1);

            if (manager.patch_task(id, status)) {
                Task task = manager.get_task_by_id(id);
                res.set_content(task.to_json(), "application/json");
            }
            else {
                res.status = 404;
                res.set_content(create_error("Task not found"), "application/json");
            }
            });
        });

    // ========== DELETE /tasks/{id} - удалить задачу ==========
    svr.Delete(R"(/tasks/(\d+))", [&manager, &mq](const Request& req, Response& res) {
        int id = stoi(req.matches[1]);
        cout << "DELETE /tasks/" << id << "\n";

        mq.push([&manager, id, &res]() {
            if (manager.delete_task(id)) {
                res.status = 204;  // No Content
            }
            else {
                res.status = 404;
                res.set_content(create_error("Task not found"), "application/json");
            }
            });
        });

    cout << "Server ready at http://localhost:8080\n";
    cout << "Endpoints:\n";
    cout << "  GET    /tasks\n";
    cout << "  POST   /tasks\n";
    cout << "  GET    /tasks/{id}\n";
    cout << "  PUT    /tasks/{id}\n";
    cout << "  PATCH  /tasks/{id}\n";
    cout << "  DELETE /tasks/{id}\n";

    svr.listen("localhost", 8080);

    mq.stop();
    if (worker.joinable()) {
        worker.join();
    }

    return 0;
}