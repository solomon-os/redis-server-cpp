cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
list(FILTER SOURCE_FILES EXCLUDE REGEX "old\\.cpp$") # backup file, has its own main()

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)

add_executable(redis ${SOURCE_FILES})

# On macOS, provide <sys/epoll.h> via a kqueue-backed shim. On Linux
# (CodeCrafters runners) the real header is used.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_include_directories(redis PRIVATE src/compat)
endif()

target_link_libraries(redis PRIVATE asio asio::asio)
target_link_libraries(redis PRIVATE Threads::Threads)

# `cmake --build <dir> --target run` — rebuild if needed, then run the server
add_custom_target(run
    COMMAND redis
    DEPENDS redis
    USES_TERMINAL
)