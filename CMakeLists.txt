cmake_minimum_required(VERSION 3.16)

project(AclassTool VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AclassProjectOptions)
include(AclassPlatform)
aclass_setup_project_options()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)
qt_standard_project_setup()

add_subdirectory(src)
add_subdirectory(dist)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
