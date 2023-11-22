cmake_minimum_required(VERSION 3.0)

project(sem_shm LANGUAGES CXX
                VERSION 0.1
                DESCRIPTION "Example of using semaphores to synchronize access to shared memory")

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src/ext)

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

target_link_libraries(server stb_image)
target_link_libraries(client stb_image)

file(COPY src/img DESTINATION ${CMAKE_BINARY_DIR})
