set(ACLASS_TARGET ${ACLASS_APP_TARGET})

add_subdirectory(network)
add_subdirectory(models)
add_subdirectory(services)
add_subdirectory(ui)

set(ACLASS_CORE_SOURCES
    main.cpp
    app_icon.rc
    ../resources/app.qrc
)

qt_add_executable(${ACLASS_TARGET}
    ${ACLASS_CORE_SOURCES}
)

target_link_libraries(${ACLASS_TARGET} PRIVATE
    AclassNetworkSources
    AclassModelSources
    AclassServiceSources
    AclassUiSources
    AclassToolUiFiles
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::QuickWidgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::SerialBus
)

target_include_directories(${ACLASS_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/services
)

set_target_properties(${ACLASS_TARGET} PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

if(WIN32)
    set_target_properties(${ACLASS_TARGET} PROPERTIES WIN32_EXECUTABLE ON)
endif()

aclass_setup_target(${ACLASS_TARGET})
aclass_install_target(${ACLASS_TARGET})
