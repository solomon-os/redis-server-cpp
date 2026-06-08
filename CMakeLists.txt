cmake_minimum_required(VERSION 3.28) # 3.28+ required for C++20 module support

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
list(FILTER SOURCE_FILES EXCLUDE REGEX "old\\.cpp$") # backup file, has its own main()

add_executable(redis ${SOURCE_FILES})
set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)


# Module scanning requires the C++ standard as a target compile feature, not
# just CMAKE_CXX_STANDARD (which isn't enough under the vcpkg toolchain).
target_compile_features(redis PRIVATE cxx_std_23)

# On macOS, provide <sys/epoll.h> via a kqueue-backed shim. On Linux
# (CodeCrafters runners) the real header is used.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_include_directories(redis PRIVATE src/compat)
endif()

target_link_libraries(redis PRIVATE Threads::Threads)

# `cmake --build <dir> --target run` — rebuild if needed, then run the server
add_custom_target(run
    COMMAND redis
    DEPENDS redis
    USES_TERMINAL
)
