add_subdirectory(theme)
add_subdirectory(pages)

set(ACLASS_UI_SOURCES
    ui/mainwindow.cpp
    ui/mainwindow.h
    ${ACLASS_UI_THEME_SOURCES}
    ${ACLASS_UI_PAGE_SOURCES}
)

set(ACLASS_UI_SOURCES
    ${ACLASS_UI_SOURCES}
    PARENT_SCOPE
)
