add_library(AclassUiThemeSources INTERFACE)

set(ACLASS_UI_THEME_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/thememanager.cpp
)

set(ACLASS_UI_THEME_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/thememanager.h
)

target_sources(AclassUiThemeSources INTERFACE
    ${ACLASS_UI_THEME_SOURCES}
    ${ACLASS_UI_THEME_HEADERS}
)
