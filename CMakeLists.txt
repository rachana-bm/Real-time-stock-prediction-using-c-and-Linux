cmake_minimum_required(VERSION 3.10)
project(stock_tracker)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories(src)

file(GLOB SOURCES "src/*.cpp")

add_executable(stock-tracker ${SOURCES})

find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

target_link_libraries(stock-tracker PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

# Filesystem fix for GCC < 10
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION
    )
    if(GCC_VERSION VERSION_LESS 10)
        target_link_libraries(stock-tracker PRIVATE stdc++fs)
    endif()
endif()

# Alert sound note
# Requires `aplay` and `sound/alert.wav` present

