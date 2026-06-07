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

enable_testing()

set(SOURCES
    src/main.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/Config.cpp
    src/ThreadPool.cpp
    src/WSAContext.cpp
    src/Socket.cpp
    src/TcpServer.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/Router.cpp
    src/MiddlewarePipeline.cpp
    src/MimeTypes.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
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
add_test(NAME HttpServerTests COMMAND HttpServerTests)

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
add_test(NAME SocketTests COMMAND SocketTests)

set(TCP_SERVER_TEST_SOURCES
    tests/TcpServerTests.cpp
    src/TcpServer.cpp
    src/Socket.cpp
    src/WSAContext.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/Router.cpp
    src/MiddlewarePipeline.cpp
    src/MimeTypes.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/Config.cpp
)
add_executable(TcpServerTests ${TCP_SERVER_TEST_SOURCES})

if(WIN32)
    target_link_libraries(TcpServerTests PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(TcpServerTests PRIVATE Threads::Threads)
endif()
add_test(NAME TcpServerTests COMMAND TcpServerTests)

set(HTTP_REQUEST_TEST_SOURCES
    tests/HttpRequestTests.cpp
    src/HttpRequest.cpp
    src/HttpMethod.cpp
)
add_executable(HttpRequestTests ${HTTP_REQUEST_TEST_SOURCES})
target_link_libraries(HttpRequestTests PRIVATE Threads::Threads)
add_test(NAME HttpRequestTests COMMAND HttpRequestTests)

set(HTTP_RESPONSE_TEST_SOURCES
    tests/HttpResponseTests.cpp
    src/HttpResponse.cpp
)
add_executable(HttpResponseTests ${HTTP_RESPONSE_TEST_SOURCES})
target_link_libraries(HttpResponseTests PRIVATE Threads::Threads)
add_test(NAME HttpResponseTests COMMAND HttpResponseTests)

set(HTTP_PARSER_TEST_SOURCES
    tests/HttpParserTests.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/HttpMethod.cpp
)
add_executable(HttpParserTests ${HTTP_PARSER_TEST_SOURCES})
target_link_libraries(HttpParserTests PRIVATE Threads::Threads)
add_test(NAME HttpParserTests COMMAND HttpParserTests)

set(INTEGRATION_TEST_SOURCES
    tests/IntegrationTests.cpp
    src/TcpServer.cpp
    src/Socket.cpp
    src/ThreadPool.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/Router.cpp
    src/MiddlewarePipeline.cpp
    src/MimeTypes.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/Config.cpp
)
add_executable(IntegrationTests ${INTEGRATION_TEST_SOURCES})
if(WIN32)
    target_link_libraries(IntegrationTests PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(IntegrationTests PRIVATE Threads::Threads)
endif()
add_test(NAME IntegrationTests COMMAND IntegrationTests)

set(ROUTER_TEST_SOURCES
    tests/RouterTests.cpp
    src/Router.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/MiddlewarePipeline.cpp
    src/MimeTypes.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/Config.cpp
    src/Logger.cpp
    src/Metrics.cpp
)
add_executable(RouterTests ${ROUTER_TEST_SOURCES})
target_link_libraries(RouterTests PRIVATE Threads::Threads)
add_test(NAME RouterTests COMMAND RouterTests)

set(MIDDLEWARE_TEST_SOURCES
    tests/MiddlewareTests.cpp
    src/MiddlewarePipeline.cpp
    src/Router.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/MimeTypes.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/Config.cpp
)
add_executable(MiddlewareTests ${MIDDLEWARE_TEST_SOURCES})
target_link_libraries(MiddlewareTests PRIVATE Threads::Threads)
add_test(NAME MiddlewareTests COMMAND MiddlewareTests)

set(STATIC_FILE_TEST_SOURCES
    tests/StaticFileTests.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/MimeTypes.cpp
    src/Config.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
)
add_executable(StaticFileTests ${STATIC_FILE_TEST_SOURCES})
target_link_libraries(StaticFileTests PRIVATE Threads::Threads)

set(KEEP_ALIVE_TEST_SOURCES
    tests/KeepAliveTests.cpp
    src/TcpServer.cpp
    src/Socket.cpp
    src/ThreadPool.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/Router.cpp
    src/MiddlewarePipeline.cpp
    src/MimeTypes.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/Config.cpp
    src/WSAContext.cpp
)
add_executable(KeepAliveTests ${KEEP_ALIVE_TEST_SOURCES})
if(WIN32)
    target_link_libraries(KeepAliveTests PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(KeepAliveTests PRIVATE Threads::Threads)
endif()

set(FILE_CACHE_TEST_SOURCES
    tests/FileCacheTests.cpp
    src/FileCache.cpp
    src/MimeTypes.cpp
    src/Config.cpp
    src/Logger.cpp
    src/Metrics.cpp
)
add_executable(FileCacheTests ${FILE_CACHE_TEST_SOURCES})
target_link_libraries(FileCacheTests PRIVATE Threads::Threads)
add_test(NAME FileCacheTests COMMAND FileCacheTests)
add_test(NAME StaticFileTests COMMAND StaticFileTests)
add_test(NAME KeepAliveTests COMMAND KeepAliveTests)

set(BENCHMARK_SOURCES
    benchmarks/BenchmarkRunner.cpp
    src/HttpParser.cpp
    src/HttpRequest.cpp
    src/Router.cpp
    src/MiddlewarePipeline.cpp
    src/HttpResponse.cpp
    src/HttpMethod.cpp
    src/ThreadPool.cpp
    src/Logger.cpp
    src/Metrics.cpp
    src/StaticFileHandler.cpp
    src/FileCache.cpp
    src/MimeTypes.cpp
    src/Config.cpp
    src/TcpServer.cpp
    src/Socket.cpp
    src/WSAContext.cpp
)
add_executable(BenchmarkRunner ${BENCHMARK_SOURCES})
if(WIN32)
    target_link_libraries(BenchmarkRunner PRIVATE ws2_32 Threads::Threads)
else()
    target_link_libraries(BenchmarkRunner PRIVATE Threads::Threads)
endif()

set(ALL_TEST_TARGETS 
    HttpServerTests SocketTests TcpServerTests HttpRequestTests 
    HttpResponseTests HttpParserTests IntegrationTests RouterTests 
    MiddlewareTests StaticFileTests FileCacheTests KeepAliveTests
)
foreach(TEST_TARGET ${ALL_TEST_TARGETS})
    if(MSVC)
        target_compile_options(${TEST_TARGET} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${TEST_TARGET} PRIVATE -UNDEBUG)
    endif()
endforeach()
