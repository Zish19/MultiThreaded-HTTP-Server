cmake_minimum_required(VERSION 3.20)
project(HttpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat warnings as errors for interview-grade code
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/Config.cpp
)

# Executable
add_executable(HttpServer ${SOURCES})

# Link Winsock on Windows, Threads on Linux
if(WIN32)
    target_link_libraries(HttpServer PRIVATE ws2_32)
else()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(HttpServer PRIVATE Threads::Threads)
endif()
