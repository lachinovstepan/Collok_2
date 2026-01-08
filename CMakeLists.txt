cmake_minimum_required(VERSION 3.15)
project(TodoApi)

set(CMAKE_CXX_STANDARD 17)

# Все исходные файлы в текущей папке
add_executable(TodoApi
    main.cpp
    task.cpp
    queue.cpp
    handler.cpp
)

# Для Windows
if(WIN32)
    target_link_libraries(TodoApi ws2_32)
endif()