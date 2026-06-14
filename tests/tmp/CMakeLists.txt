cmake_minimum_required(VERSION 3.16)
project(binproto_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(binproto_test
    test_binprotocol_main.cpp
    ${CMAKE_SOURCE_DIR}/../../src/models/binprotocolloader.cpp
    ${CMAKE_SOURCE_DIR}/../../src/models/framecodec.cpp
)
target_include_directories(binproto_test PRIVATE
    ${CMAKE_SOURCE_DIR}/../../src
    ${CMAKE_SOURCE_DIR}/../../src/models
)
target_link_libraries(binproto_test PRIVATE Qt6::Core)
