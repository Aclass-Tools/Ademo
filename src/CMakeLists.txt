set(APP_SOURCES
    main.cpp
    app_icon.rc
    network/apiclient.cpp
    network/apiclient.h
    network/websocketclient.cpp
    network/websocketclient.h
    models/pointtablemodel.cpp
    models/pointtablemodel.h
    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/theme/thememanager.cpp
    ui/theme/thememanager.h
    ui/pages/placeholderpagebase.cpp
    ui/pages/placeholderpagebase.h
    ui/pages/homepage.cpp
    ui/pages/homepage.h
    ui/pages/protocoleditorpage.cpp
    ui/pages/protocoleditorpage.h
    ui/pages/protocoldebugpage.cpp
    ui/pages/protocoldebugpage.h
    ui/pages/protocolexportpage.cpp
    ui/pages/protocolexportpage.h
    ui/pages/deviceupgradepage.cpp
    ui/pages/deviceupgradepage.h
    ui/pages/terminalpage.cpp
    ui/pages/terminalpage.h
    ui/pages/logpage.cpp
    ui/pages/logpage.h
    ui/pages/pluginpage.cpp
    ui/pages/pluginpage.h
    ../ui/mainwindow.ui
    ../ui/homepage.ui
    ../ui/protocoleditorpage.ui
    ../ui/protocoldebugpage.ui
    ../ui/protocolexport.ui
    ../ui/deviceupgrade.ui
    ../ui/terminalpage.ui
    ../ui/logpage.ui
    ../ui/pluginpage.ui
    ../resources/app.qrc
)

qt_add_executable(AclassTool
    ${APP_SOURCES}
)

target_link_libraries(AclassTool PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
)

target_include_directories(AclassTool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

set_target_properties(AclassTool PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

if(WIN32)
    set_target_properties(AclassTool PROPERTIES WIN32_EXECUTABLE ON)
endif()

aclass_setup_target(AclassTool)
aclass_install_target(AclassTool)
