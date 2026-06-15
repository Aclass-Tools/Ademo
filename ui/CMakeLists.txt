# UI assets are exposed through a lightweight interface target.
add_library(AclassToolUiFiles INTERFACE)

set(ACLASS_UI_FILES
    ${CMAKE_CURRENT_LIST_DIR}/mainwindow.ui
    ${CMAKE_CURRENT_LIST_DIR}/homepage.ui
    ${CMAKE_CURRENT_LIST_DIR}/protocoleditorpage.ui
    ${CMAKE_CURRENT_LIST_DIR}/protocoldebugpage.ui
    ${CMAKE_CURRENT_LIST_DIR}/protocolexport.ui
    ${CMAKE_CURRENT_LIST_DIR}/deviceupgrade.ui
    ${CMAKE_CURRENT_LIST_DIR}/terminalpage.ui
    ${CMAKE_CURRENT_LIST_DIR}/logpage.ui
    ${CMAKE_CURRENT_LIST_DIR}/pluginpage.ui
    ${CMAKE_CURRENT_LIST_DIR}/ConnectionPage.ui
    ${CMAKE_CURRENT_LIST_DIR}/CommandOperationPage.ui
)

# Let consumers get all .ui inputs transitively.
target_sources(AclassToolUiFiles INTERFACE
    ${ACLASS_UI_FILES}
)
