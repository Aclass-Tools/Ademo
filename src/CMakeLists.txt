set(ACLASS_TARGET ${ACLASS_APP_TARGET})

add_subdirectory(network)
add_subdirectory(models)
add_subdirectory(ui)

set(ACLASS_CORE_SOURCES
    main.cpp
    app_icon.rc
)

set(ACLASS_RESOURCE_SOURCES
    ../resources/app.qrc
)

qt_add_executable(${ACLASS_TARGET}
    ${ACLASS_CORE_SOURCES}
    ${ACLASS_NETWORK_SOURCES}
    ${ACLASS_MODEL_SOURCES}
    ${ACLASS_UI_SOURCES}
    ${ACLASS_RESOURCE_SOURCES}
)

target_link_libraries(${ACLASS_TARGET} PRIVATE
    AclassToolUiFiles
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
    Qt${QT_VERSION_MAJOR}::QuickWidgets
)

target_include_directories(${ACLASS_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

set_target_properties(${ACLASS_TARGET} PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_SOURCE_DIR}/ui"
)

if(WIN32)
    set_target_properties(${ACLASS_TARGET} PROPERTIES WIN32_EXECUTABLE ON)
endif()

aclass_setup_target(${ACLASS_TARGET})
aclass_install_target(${ACLASS_TARGET})
