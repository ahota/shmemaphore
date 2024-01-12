cmake_minimum_required(VERSION 3.5)

project(shmemaphore LANGUAGES CXX
                    VERSION 0.1
                    DESCRIPTION "Example of using semaphores to synchronize access to shared memory")

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(ext)

add_executable(server app/server.cpp)
target_include_directories(server PUBLIC src ext)
add_executable(client app/client.cpp)
target_include_directories(client PUBLIC src ext)

target_link_libraries(server stb_image)
target_link_libraries(client stb_image)

file(COPY app/img DESTINATION ${CMAKE_BINARY_DIR})
