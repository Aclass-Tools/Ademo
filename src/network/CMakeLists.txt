add_library(AclassNetworkSources INTERFACE)

set(ACLASS_NETWORK_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/apiclient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/websocketclient.cpp
)

set(ACLASS_NETWORK_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/apiclient.h
    ${CMAKE_CURRENT_LIST_DIR}/websocketclient.h
)

target_sources(AclassNetworkSources INTERFACE
    ${ACLASS_NETWORK_SOURCES}
    ${ACLASS_NETWORK_HEADERS}
)
