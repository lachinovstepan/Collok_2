// httplib.h - УПРОЩЕННАЯ РАБОЧАЯ ВЕРСИЯ
#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <regex>
#include <map>
#include <thread>
#include <atomic>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

namespace httplib {

#ifdef _WIN32
    typedef SOCKET socket_t;
    inline socket_t invalid_socket = INVALID_SOCKET;
#else
    typedef int socket_t;
    inline socket_t invalid_socket = -1;
#endif

    struct Request {
        std::string method;
        std::string path;
        std::string body;
        std::smatch matches;
        std::map<std::string, std::string> headers;
    };

    struct Response {
        int status = 200;
        std::string body;
        std::map<std::string, std::string> headers;

        void set_content(const std::string& s, const std::string& content_type) {
            body = s;
            headers["Content-Type"] = content_type;
        }
    };

    class Server {
    public:
        using Handler = std::function<void(const Request&, Response&)>;

        Server() {
#ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        }

        ~Server() {
            stop();
#ifdef _WIN32
            WSACleanup();
#endif
        }

        Server& Get(const std::string& pattern, Handler handler) {
            get_handlers_.push_back({ pattern, handler });
            return *this;
        }

        Server& Post(const std::string& pattern, Handler handler) {
            post_handlers_.push_back({ pattern, handler });
            return *this;
        }

        Server& Put(const std::string& pattern, Handler handler) {
            put_handlers_.push_back({ pattern, handler });
            return *this;
        }

        Server& Patch(const std::string& pattern, Handler handler) {
            patch_handlers_.push_back({ pattern, handler });
            return *this;
        }

        Server& Delete(const std::string& pattern, Handler handler) {
            delete_handlers_.push_back({ pattern, handler });
            return *this;
        }

        bool listen(const std::string& host, int port) {
            // Создаем сокет
            socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd == invalid_socket) {
                std::cerr << "Socket creation failed" << std::endl;
                return false;
            }

            // Настройка адреса
            sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_port = htons(port);

            if (host == "localhost" || host == "127.0.0.1") {
                address.sin_addr.s_addr = INADDR_ANY;
            }
            else {
                inet_pton(AF_INET, host.c_str(), &address.sin_addr);
            }

            // Биндим сокет
            int opt = 1;
#ifdef _WIN32
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

            if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
                std::cerr << "Bind failed" << std::endl;
#ifdef _WIN32
                closesocket(server_fd);
#else
                close(server_fd);
#endif
                return false;
            }

            // Слушаем порт
            if (::listen(server_fd, 3) < 0) {
                std::cerr << "Listen failed" << std::endl;
#ifdef _WIN32
                closesocket(server_fd);
#else
                close(server_fd);
#endif
                return false;
            }

            std::cout << "Server listening on http://" << host << ":" << port << std::endl;
            std::cout << "Press Ctrl+C to stop" << std::endl;

            running_ = true;

            // Основной цикл сервера
            while (running_) {
                sockaddr_in client_addr;
#ifdef _WIN32
                int addrlen = sizeof(client_addr);
#else
                socklen_t addrlen = sizeof(client_addr);
#endif

                socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &addrlen);

                if (client_fd == invalid_socket) {
                    if (running_) {
                        std::cerr << "Accept failed" << std::endl;
                    }
                    continue;
                }

                // Обрабатываем запрос в отдельном потоке
                std::thread([this, client_fd]() {
                    handle_client(client_fd);
                    }).detach();
            }

#ifdef _WIN32
            closesocket(server_fd);
#else
            close(server_fd);
#endif

            return true;
        }

        void stop() {
            running_ = false;
            // Для прерывания accept() нужно закрыть сокет, но в упрощенной версии просто выходим
        }

    private:
        void handle_client(socket_t client_fd) {
            char buffer[4096] = { 0 };

#ifdef _WIN32
            int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#else
            ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#endif

            if (bytes_received <= 0) {
#ifdef _WIN32
                closesocket(client_fd);
#else
                close(client_fd);
#endif
                return;
            }

            buffer[bytes_received] = '\0';
            std::string request_str(buffer);

            // Парсим HTTP запрос
            Request req;
            Response res;

            // Извлекаем метод и путь
            size_t method_end = request_str.find(' ');
            if (method_end != std::string::npos) {
                req.method = request_str.substr(0, method_end);

                size_t path_start = method_end + 1;
                size_t path_end = request_str.find(' ', path_start);
                if (path_end != std::string::npos) {
                    req.path = request_str.substr(path_start, path_end - path_start);
                }
            }

            // Извлекаем тело запроса
            size_t body_start = request_str.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                req.body = request_str.substr(body_start + 4);
            }

            // Убираем параметры запроса из пути
            size_t query_pos = req.path.find('?');
            if (query_pos != std::string::npos) {
                req.path = req.path.substr(0, query_pos);
            }

            // Вызываем соответствующий обработчик
            bool handled = false;

            if (req.method == "GET") {
                handled = try_handle(get_handlers_, req, res);
            }
            else if (req.method == "POST") {
                handled = try_handle(post_handlers_, req, res);
            }
            else if (req.method == "PUT") {
                handled = try_handle(put_handlers_, req, res);
            }
            else if (req.method == "PATCH") {
                handled = try_handle(patch_handlers_, req, res);
            }
            else if (req.method == "DELETE") {
                handled = try_handle(delete_handlers_, req, res);
            }

            if (!handled) {
                res.status = 404;
                res.set_content("{\"error\":\"Not found\"}", "application/json");
            }

            // Формируем HTTP ответ
            std::string response_str = "HTTP/1.1 " + std::to_string(res.status) + " OK\r\n";
            response_str += "Content-Type: " + res.headers["Content-Type"] + "\r\n";
            response_str += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";
            response_str += "Connection: close\r\n\r\n";
            response_str += res.body;

            // Отправляем ответ
#ifdef _WIN32
            send(client_fd, response_str.c_str(), response_str.size(), 0);
            closesocket(client_fd);
#else
            send(client_fd, response_str.c_str(), response_str.size(), 0);
            close(client_fd);
#endif
        }

        bool try_handle(const std::vector<std::pair<std::string, Handler>>& handlers,
            const Request& req, Response& res) {
            for (const auto& [pattern, handler] : handlers) {
                std::regex re(pattern);
                std::smatch matches;

                if (std::regex_match(req.path, matches, re)) {
                    // Копируем matches в req для передачи в handler
                    Request req_copy = req;
                    req_copy.matches = matches;
                    handler(req_copy, res);
                    return true;
                }
            }
            return false;
        }
        std::vector<std::pair<std::string, Handler>> get_handlers_;
        std::vector<std::pair<std::string, Handler>> post_handlers_;
        std::vector<std::pair<std::string, Handler>> put_handlers_;
        std::vector<std::pair<std::string, Handler>> patch_handlers_;
        std::vector<std::pair<std::string, Handler>> delete_handlers_;
        std::atomic<bool> running_{ false };
    };

} // namespace httplib