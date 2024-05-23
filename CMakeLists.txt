cmake_minimum_required(VERSION 3.5)

project(shmemaphore LANGUAGES CXX
                    VERSION 0.1
                    DESCRIPTION "Example of using semaphores to synchronize access to shared memory")

set(CMAKE_CXX_STANDARD 14)

option(BUILD_APP "Build test applications" OFF)

if(BUILD_APP)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  add_subdirectory(ext)

  add_executable(server app/server.cpp)
  target_include_directories(server PUBLIC src ext)
  add_executable(client app/client.cpp)
  target_include_directories(client PUBLIC src ext)

  target_link_libraries(server stb_image rt Threads::Threads)
  target_link_libraries(client stb_image rt Threads::Threads)

  file(COPY app/img DESTINATION ${CMAKE_BINARY_DIR})
endif()

add_library(shm4 SHARED src/shmemaphore.h)
set_target_properties(shm4 PROPERTIES LINKER_LANGUAGE CXX)
