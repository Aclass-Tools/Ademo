set(APP_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    ../ui/mainwindow.ui
    ../resources/app.qrc
)

qt_add_executable(AclassTool
    ${APP_SOURCES}
)

target_link_libraries(AclassTool PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
set_target_properties(AclassTool PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

aclass_setup_target(AclassTool)
aclass_install_target(AclassTool)
