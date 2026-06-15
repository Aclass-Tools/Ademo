add_library(AclassUiPageSources INTERFACE)

set(ACLASS_UI_PAGE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/placeholderpagebase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/homepage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocoleditorpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocoldebugpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocolexportpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/deviceupgradepage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/terminalpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pluginpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/connectionpage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/commandoperationpage.cpp
)

set(ACLASS_UI_PAGE_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/placeholderpagebase.h
    ${CMAKE_CURRENT_LIST_DIR}/homepage.h
    ${CMAKE_CURRENT_LIST_DIR}/protocoleditorpage.h
    ${CMAKE_CURRENT_LIST_DIR}/protocoldebugpage.h
    ${CMAKE_CURRENT_LIST_DIR}/protocolexportpage.h
    ${CMAKE_CURRENT_LIST_DIR}/deviceupgradepage.h
    ${CMAKE_CURRENT_LIST_DIR}/terminalpage.h
    ${CMAKE_CURRENT_LIST_DIR}/logpage.h
    ${CMAKE_CURRENT_LIST_DIR}/pluginpage.h
    ${CMAKE_CURRENT_LIST_DIR}/connectionpage.h
    ${CMAKE_CURRENT_LIST_DIR}/commandoperationpage.h
)

target_sources(AclassUiPageSources INTERFACE
    ${ACLASS_UI_PAGE_SOURCES}
    ${ACLASS_UI_PAGE_HEADERS}
)
