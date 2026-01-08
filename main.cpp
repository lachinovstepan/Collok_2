#include "handler.h"
#include "queue.h"
#include <httplib.h>
#include <iostream>

using json = nlohmann::json;
using namespace httplib;

int main() {
    MessageQueue mq;
    TaskManager manager(mq);

    // Запуск очереди в отдельном потоке
    std::thread worker([&mq]() { mq.run(); });

    Server svr;

    // GET /tasks
    svr.Get("/tasks", [&](const Request& req, Response& res) {
        auto all_tasks = manager.get_all_tasks();
        json response = json::array();
        for (const auto& t : all_tasks) {
            response.push_back(t.to_json());
        }
        res.set_content(response.dump(), "application/json");
        });

    // POST /tasks
    svr.Post("/tasks", [&](const Request& req, Response& res) {
        auto j = json::parse(req.body);
        Task task = Task::from_json(j);

        mq.push([&manager, task, &res]() {
            int id = manager.create_task(task);
            json response = task.to_json();
            response["id"] = id;
            res.set_content(response.dump(), "application/json");
            res.status = 201;
            });
        });

    // GET /tasks/{id}
    svr.Get(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        Task task = manager.get_task_by_id(id);
        if (task.id == 0) {
            res.status = 404;
            return;
        }
        res.set_content(task.to_json().dump(), "application/json");
        });

    // PUT /tasks/{id}
    svr.Put(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        auto j = json::parse(req.body);
        Task task = Task::from_json(j);

        mq.push([&manager, id, task, &res]() {
            if (manager.update_task(id, task)) {
                res.set_content(task.to_json().dump(), "application/json");
            }
            else {
                res.status = 404;
            }
            });
        });

    // DELETE /tasks/{id}
    svr.Delete(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);

        mq.push([&manager, id, &res]() {
            if (manager.delete_task(id)) {
                res.status = 204;
            }
            else {
                res.status = 404;
            }
            });
        });

    std::cout << "Server started at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);

    mq.stop();
    worker.join();

    return 0;
}