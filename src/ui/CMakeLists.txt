add_subdirectory(theme)
add_subdirectory(pages)

add_library(AclassUiSources INTERFACE)

set(ACLASS_UI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/mainwindow.cpp
)

set(ACLASS_UI_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/mainwindow.h
)

target_sources(AclassUiSources INTERFACE
    ${ACLASS_UI_SOURCES}
    ${ACLASS_UI_HEADERS}
)

target_link_libraries(AclassUiSources INTERFACE
    AclassUiThemeSources
    AclassUiPageSources
)
