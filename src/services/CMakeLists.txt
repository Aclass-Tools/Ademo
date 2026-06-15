# services 层：统一连接管理 + 传输封装协议（A0/A3/Modbus）。
add_library(AclassServiceSources INTERFACE)

set(ACLASS_SERVICE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/protocolparser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/serialservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/networkservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/connectionmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/modbusservice.cpp
)

set(ACLASS_SERVICE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/protocolparser.h
    ${CMAKE_CURRENT_LIST_DIR}/serialservice.h
    ${CMAKE_CURRENT_LIST_DIR}/networkservice.h
    ${CMAKE_CURRENT_LIST_DIR}/connectionmanager.h
    ${CMAKE_CURRENT_LIST_DIR}/modbusservice.h
)

target_sources(AclassServiceSources INTERFACE
    ${ACLASS_SERVICE_SOURCES}
    ${ACLASS_SERVICE_HEADERS}
)
