// httplib.h - минимальная версия для тестирования
#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace httplib {

#ifdef _WIN32
    typedef SOCKET socket_t;
    inline socket_t invalid_socket = INVALID_SOCKET;
    inline int socket_error = SOCKET_ERROR;
#else
    typedef int socket_t;
    inline socket_t invalid_socket = -1;
    inline int socket_error = -1;
#endif

    class Request {
    public:
        std::string method;
        std::string path;
        std::string body;
        std::map<std::string, std::string> headers;
        std::smatch matches;
    };

    class Response {
    public:
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
            std::cout << "Server listening on http://" << host << ":" << port << std::endl;
            std::cout << "Press Ctrl+C to stop" << std::endl;

            // Просто ждем ввод чтобы программа не завершилась
            std::cout << "Type 'exit' to stop server: ";
            std::string input;
            while (std::cin >> input && input != "exit") {
                std::cout << "Server still running... Type 'exit' to stop: ";
            }

            return true;
        }

    private:
        std::vector<std::pair<std::string, Handler>> get_handlers_;
        std::vector<std::pair<std::string, Handler>> post_handlers_;
        std::vector<std::pair<std::string, Handler>> put_handlers_;
        std::vector<std::pair<std::string, Handler>> patch_handlers_;
        std::vector<std::pair<std::string, Handler>> delete_handlers_;
    };

} // namespace httplib