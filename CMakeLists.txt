cmake_minimum_required(VERSION 3.20)
project(HttpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/Config.cpp
    src/ThreadPool.cpp
    src/WSAContext.cpp
    src/Socket.cpp
    src/TcpServer.cpp
)

add_executable(HttpServer ${SOURCES})

if(WIN32)
    target_link_libraries(HttpServer PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(HttpServer PRIVATE Threads::Threads)
endif()

set(TEST_SOURCES
    tests/ThreadPoolTests.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
    src/Metrics.cpp
)
add_executable(HttpServerTests ${TEST_SOURCES})
target_link_libraries(HttpServerTests PRIVATE Threads::Threads)

set(SOCKET_TEST_SOURCES
    tests/SocketTests.cpp
    src/Socket.cpp
    src/WSAContext.cpp
)
add_executable(SocketTests ${SOCKET_TEST_SOURCES})

if(WIN32)
    target_link_libraries(SocketTests PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(SocketTests PRIVATE Threads::Threads)
endif()

set(TCP_SERVER_TEST_SOURCES
    tests/TcpServerTests.cpp
    src/TcpServer.cpp
    src/Socket.cpp
    src/WSAContext.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
    src/Metrics.cpp
)
add_executable(TcpServerTests ${TCP_SERVER_TEST_SOURCES})

if(WIN32)
    target_link_libraries(TcpServerTests PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(TcpServerTests PRIVATE Threads::Threads)
endif()

set(HTTP_REQUEST_TEST_SOURCES
    tests/HttpRequestTests.cpp
    src/HttpRequest.cpp
    src/HttpMethod.cpp
)
add_executable(HttpRequestTests ${HTTP_REQUEST_TEST_SOURCES})
target_link_libraries(HttpRequestTests PRIVATE Threads::Threads)

set(HTTP_RESPONSE_TEST_SOURCES
    tests/HttpResponseTests.cpp
    src/HttpResponse.cpp
)
add_executable(HttpResponseTests ${HTTP_RESPONSE_TEST_SOURCES})
target_link_libraries(HttpResponseTests PRIVATE Threads::Threads)

set(HTTP_PARSER_TEST_SOURCES
    tests/HttpParserTests.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/HttpMethod.cpp
)
add_executable(HttpParserTests ${HTTP_PARSER_TEST_SOURCES})
target_link_libraries(HttpParserTests PRIVATE Threads::Threads)
