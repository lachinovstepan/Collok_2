cmake_minimum_required(VERSION 3.10)
project(TodoApiCpp)

set(CMAKE_CXX_STANDARD 17)

# Загрузка библиотек через FetchContent или find_package
find_package(nlohmann_json 3.9.1 REQUIRED)
find_package(httplib REQUIRED)

# Или загрузка через FetchContent:
# include(FetchContent)
# FetchContent_Declare(httplib GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git)
# FetchContent_MakeAvailable(httplib)

add_executable(todo_api src/main.cpp src/task.cpp sr "task.h"c/queue.cpp src/handler.cpp)

target_include_directories(todo_api PRIVATE include)
target_link_libraries(todo_api PRIVATE nlohmann_json::nlohmann_json httplib::httplib)