#include "handler.h"
#include "queue.h"
#include "task.h"
#include "httplib.h"
#include <iostream>
#include <sstream>

using namespace httplib;
using namespace std;

// Простая функция для логирования операций через очередь сообщений
void log_operation(MessageQueue& mq, const string& operation) {
    mq.push([operation]() {
        cout << "[QUEUE LOG] " << operation << endl;
        });
}

// Функция для создания JSON ошибки
string create_error(const string& message) {
    return "{\"error\":\"" + message + "\"}";
}

int main() {
    setlocale (LC_ALL, "RUS");
    cout << "=== To-Do API Server ===\n";

    // Создаем очередь сообщений для логирования операций
    MessageQueue log_queue;

    // Запускаем обработку очереди логов в отдельном потоке
    thread log_worker([&log_queue]() {
        log_queue.run();
        });

    // Создаем менеджер задач (передаем ему очередь для демонстрации)
    TaskManager manager(log_queue);

    Server svr;

    // ========== GET /tasks - все задачи ==========
    svr.Get("/tasks", [&manager, &log_queue](const Request& req, Response& res) {
        cout << "GET /tasks\n";
        log_operation(log_queue, "GET /tasks - Получение всех задач");

        auto tasks = manager.get_all_tasks();
        string result = "[";
        for (size_t i = 0; i < tasks.size(); i++) {
            result += tasks[i].to_json();
            if (i < tasks.size() - 1) result += ",";
        }
        result += "]";
        res.set_content(result, "application/json");
        });

    // ========== POST /tasks - создать задачу (СИНХРОННО) ==========
    svr.Post("/tasks", [&manager, &log_queue](const Request& req, Response& res) {
        cout << "POST /tasks\n";

        if (req.body.empty()) {
            res.status = 400;
            res.set_content(create_error("Пустое тело запроса"), "application/json");
            return;
        }

        try {
            Task new_task = Task::from_json(req.body);

            if (new_task.title.empty()) {
                res.status = 400;
                res.set_content(create_error("Заголовок задачи обязателен"), "application/json");
                return;
            }

            // СИНХРОННО создаем задачу
            int task_id = manager.create_task(new_task);
            new_task.id = task_id;

            res.status = 201;  // Created
            res.set_content(new_task.to_json(), "application/json");

            // Асинхронно логируем операцию через очередь
            log_operation(log_queue, "POST /tasks - Создана задача #" + to_string(task_id));

            cout << "  -> Создана задача #" << task_id << endl;
        }
        catch (const exception& e) {
            res.status = 400;
            res.set_content(create_error("Неверный JSON формат"), "application/json");
        }
        });

    // ========== GET /tasks/{id} ==========
    svr.Get(R"(/tasks/(\d+))", [&manager, &log_queue](const Request& req, Response& res) {
        int task_id = stoi(req.matches[1]);
        cout << "GET /tasks/" << task_id << "\n";
        log_operation(log_queue, "GET /tasks/" + to_string(task_id) + " - Получение задачи");

        Task task = manager.get_task_by_id(task_id);

        if (task.id == 0) {
            res.status = 404;
            res.set_content(create_error("Задача не найдена"), "application/json");
            return;
        }

        res.set_content(task.to_json(), "application/json");
        });

    // ========== PUT /tasks/{id} - обновить задачу (СИНХРОННО) ==========
    svr.Put(R"(/tasks/(\d+))", [&manager, &log_queue](const Request& req, Response& res) {
        int task_id = stoi(req.matches[1]);
        cout << "PUT /tasks/" << task_id << "\n";

        if (req.body.empty()) {
            res.status = 400;
            res.set_content(create_error("Пустое тело запроса"), "application/json");
            return;
        }

        try {
            Task updated_task = Task::from_json(req.body);
            updated_task.id = task_id;

            if (updated_task.title.empty()) {
                res.status = 400;
                res.set_content(create_error("Заголовок задачи обязателен"), "application/json");
                return;
            }

            // СИНХРОННО обновляем задачу
            if (manager.update_task(task_id, updated_task)) {
                res.set_content(updated_task.to_json(), "application/json");
                log_operation(log_queue, "PUT /tasks/" + to_string(task_id) + " - Задача обновлена");
                cout << "  -> Задача #" << task_id << " обновлена" << endl;
            }
            else {
                res.status = 404;
                res.set_content(create_error("Задача не найдена"), "application/json");
            }
        }
        catch (const exception& e) {
            res.status = 400;
            res.set_content(create_error("Неверный JSON формат"), "application/json");
        }
        });

    // ========== PATCH /tasks/{id} - обновить статус (СИНХРОННО) ==========
    svr.Patch(R"(/tasks/(\d+))", [&manager, &log_queue](const Request& req, Response& res) {
        int task_id = stoi(req.matches[1]);
        cout << "PATCH /tasks/" << task_id << "\n";

        if (req.body.empty()) {
            res.status = 400;
            res.set_content(create_error("Пустое тело запроса"), "application/json");
            return;
        }

        try {
            // Простой парсинг JSON для получения статуса
            string body = req.body;
            size_t pos = body.find("\"status\"");

            if (pos == string::npos) {
                res.status = 400;
                res.set_content(create_error("Поле 'status' обязательно"), "application/json");
                return;
            }

            size_t start = body.find(':', pos);
            size_t quote1 = body.find('"', start);
            size_t quote2 = body.find('"', quote1 + 1);

            if (quote1 == string::npos || quote2 == string::npos) {
                res.status = 400;
                res.set_content(create_error("Неверный JSON формат"), "application/json");
                return;
            }

            string new_status = body.substr(quote1 + 1, quote2 - quote1 - 1);

            // СИНХРОННО обновляем статус
            if (manager.patch_task(task_id, new_status)) {
                Task updated_task = manager.get_task_by_id(task_id);
                res.set_content(updated_task.to_json(), "application/json");
                log_operation(log_queue, "PATCH /tasks/" + to_string(task_id) +
                    " - Статус изменен на: " + new_status);
                cout << "  -> Статус задачи #" << task_id
                    << " изменен на: " << new_status << endl;
            }
            else {
                res.status = 404;
                res.set_content(create_error("Задача не найдена"), "application/json");
            }
        }
        catch (const exception& e) {
            res.status = 400;
            res.set_content(create_error("Неверный JSON формат"), "application/json");
        }
        });

    // ========== DELETE /tasks/{id} - удалить задачу (СИНХРОННО) ==========
    svr.Delete(R"(/tasks/(\d+))", [&manager, &log_queue](const Request& req, Response& res) {
        int task_id = stoi(req.matches[1]);
        cout << "DELETE /tasks/" << task_id << "\n";

        // СИНХРОННО удаляем задачу
        if (manager.delete_task(task_id)) {
            res.status = 204;  // No Content
            log_operation(log_queue, "DELETE /tasks/" + to_string(task_id) + " - Задача удалена");
            cout << "  -> Задача #" << task_id << " удалена" << endl;
        }
        else {
            res.status = 404;
            res.set_content(create_error("Задача не найдена"), "application/json");
        }
        });

    // ========== GET / - главная страница ==========
    svr.Get("/", [](const Request& req, Response& res) {
        string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>To-Do API</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .endpoint { background: #f5f5f5; padding: 15px; margin: 10px 0; border-radius: 5px; }
        .method { display: inline-block; padding: 5px 10px; border-radius: 3px; color: white; font-weight: bold; }
        .get { background: #61affe; }
        .post { background: #49cc90; }
        .put { background: #fca130; }
        .patch { background: #50e3c2; }
        .delete { background: #f93e3e; }
    </style>
</head>
<body>
    <h1>📝 To-Do API Server</h1>
    <p>REST API для управления списком задач с очередью сообщений</p>
    
    <div class="endpoint">
        <span class="method get">GET</span> <strong>/tasks</strong><br>
        Получить список всех задач
    </div>
    
    <div class="endpoint">
        <span class="method post">POST</span> <strong>/tasks</strong><br>
        Создать новую задачу<br>
        Пример: {"title": "Задача", "description": "Описание", "status": "todo"}
    </div>
    
    <div class="endpoint">
        <span class="method get">GET</span> <strong>/tasks/{id}</strong><br>
        Получить задачу по ID
    </div>
    
    <div class="endpoint">
        <span class="method put">PUT</span> <strong>/tasks/{id}</strong><br>
        Полностью обновить задачу
    </div>
    
    <div class="endpoint">
        <span class="method patch">PATCH</span> <strong>/tasks/{id}</strong><br>
        Обновить статус задачи<br>
        Пример: {"status": "in_progress"}
    </div>
    
    <div class="endpoint">
        <span class="method delete">DELETE</span> <strong>/tasks/{id}</strong><br>
        Удалить задачу по ID
    </div>
    
    <p><strong>Особенность:</strong> Все операции логируются через очередь сообщений</p>
</body>
</html>
        )";

        res.set_content(html, "text/html");
        });

    cout << "Сервер запущен на http://localhost:8080" << endl;
    cout << "Документация: http://localhost:8080/" << endl;
    cout << "\nДоступные эндпоинты:" << endl;
    cout << "  GET    /tasks           - Все задачи" << endl;
    cout << "  POST   /tasks           - Создать задачу" << endl;
    cout << "  GET    /tasks/{id}      - Задача по ID" << endl;
    cout << "  PUT    /tasks/{id}      - Обновить задачу" << endl;
    cout << "  PATCH  /tasks/{id}      - Обновить статус" << endl;
    cout << "  DELETE /tasks/{id}      - Удалить задачу" << endl;
    cout << "\nНажмите Ctrl+C для остановки сервера\n" << endl;

    // Запуск сервера
    svr.listen("localhost", 8080);

    // Останавливаем очередь логов
    log_queue.stop();
    if (log_worker.joinable()) {
        log_worker.join();
    }

    return 0;
}