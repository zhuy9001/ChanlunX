cmake_minimum_required(VERSION 3.20)
project (ChanlunX)
set (ChanlunX_VERSION_MAJOR 1)
set (ChanlunX_VERSION_MINOR 0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

aux_source_directory(. SRCS)
add_library(ChanlunX SHARED ${SRCS})

option(CHANLUNX_BUILD_TESTS "Build ChanlunX core tests" ON)

if(CHANLUNX_BUILD_TESTS)
    enable_testing()
    add_executable(ChanlunCoreTests
        tests/ChanlunCoreTests.cpp
        ChanlunCore.cpp
        ChanlunSignals.cpp
        ChanlunTdx.cpp
    )
    target_include_directories(ChanlunCoreTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(ChanlunCoreTests PRIVATE CHANLUN_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME ChanlunCoreTests COMMAND ChanlunCoreTests)
endif()
