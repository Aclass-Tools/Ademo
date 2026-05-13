set(DIST_ROOT_DIR "${CMAKE_SOURCE_DIR}/dist")
set(DIST_DEBUG_DIR "${DIST_ROOT_DIR}/debug")
set(DIST_RELEASE_DIR "${DIST_ROOT_DIR}/release")
set(DIST_INSTALLER_DIR "${DIST_ROOT_DIR}/installer")

add_custom_target(dist_clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIST_DEBUG_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIST_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${DIST_INSTALLER_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DEBUG_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_RELEASE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_INSTALLER_DIR}"
    COMMENT "Clean and recreate dist directories"
    VERBATIM
)

if(WIN32)
    # Single-config generators (Ninja/Makefiles) provide CMAKE_BUILD_TYPE at configure time.
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(DIST_RUNTIME_DIR "${DIST_RELEASE_DIR}")
    else()
        set(DIST_RUNTIME_DIR "${DIST_DEBUG_DIR}")
    endif()

    if(NOT WINDEPLOYQT_EXECUTABLE)
        add_custom_target(dist_win
            COMMAND ${CMAKE_COMMAND} -E echo "WINDEPLOYQT_EXECUTABLE is not set. Configure it in .vscode/settings.json -> cmake.configureSettings."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DEBUG_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_RELEASE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_INSTALLER_DIR}"
            DEPENDS ${ACLASS_APP_TARGET}
            VERBATIM
        )
    elseif(NOT EXISTS "${WINDEPLOYQT_EXECUTABLE}")
        add_custom_target(dist_win
            COMMAND ${CMAKE_COMMAND} -E echo "WINDEPLOYQT_EXECUTABLE does not exist: ${WINDEPLOYQT_EXECUTABLE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DEBUG_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_RELEASE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_INSTALLER_DIR}"
            DEPENDS ${ACLASS_APP_TARGET}
            VERBATIM
        )
    else()
        add_custom_target(dist_win
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DEBUG_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_RELEASE_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_INSTALLER_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_RUNTIME_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${ACLASS_APP_TARGET}>" "${DIST_RUNTIME_DIR}/"
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "${DIST_RUNTIME_DIR}/$<TARGET_FILE_NAME:${ACLASS_APP_TARGET}>"
            DEPENDS ${ACLASS_APP_TARGET}
            COMMENT "Create Windows dist package in ${DIST_RUNTIME_DIR} using windeployqt"
            VERBATIM
        )
    endif()

    # CPack + NSIS installer configuration.
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-windows-${CMAKE_BUILD_TYPE}")
    set(CPACK_MONOLITHIC_INSTALL ON)
    set(CPACK_OUTPUT_FILE_PREFIX "${DIST_INSTALLER_DIR}")
    # Package the already deployed runtime directory.
    set(CPACK_INSTALLED_DIRECTORIES "${DIST_RELEASE_DIR};.")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
    # 瀹夎鍣ㄥ浘鏍囦笌鍗歌浇绋嬪簭鍥炬爣浣跨敤缁熶竴 .ico 璧勬簮銆?    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icons/app_icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icons/app_icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "AclassTool.exe")
    # Keep InstallOptions page enabled for desktop shortcut checkbox.
    # PATH radio options are hidden by custom NSIS.InstallOptions.ini template.
    set(CPACK_NSIS_MODIFY_PATH ON)
    # Create shortcuts explicitly to the deployed root executable.
    # Keep these as newline-delimited strings (not CMake lists) so NSIS receives
    # one command per line, avoiding broken semicolon concatenation.
    set(CPACK_NSIS_CREATE_ICONS_EXTRA [[
CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\AclassTool.lnk" "$INSTDIR\AclassTool.exe" "" "$INSTDIR\resources\icons\app_icon.ico" 0
StrCmp $INSTALL_DESKTOP 1 0 +2
CreateShortCut "$DESKTOP\AclassTool.lnk" "$INSTDIR\AclassTool.exe" "" "$INSTDIR\resources\icons\app_icon.ico" 0
]])
    set(CPACK_NSIS_DELETE_ICONS_EXTRA [[
Delete "$SMPROGRAMS\$MUI_TEMP\AclassTool.lnk"
Delete "$SMPROGRAMS\$START_MENU\AclassTool.lnk"
Delete "$DESKTOP\AclassTool.lnk"
]])
    # Keep only desktop checkbox on the install options page. PATH radio controls
    # are still read by CPack's NSIS template, so we force them to "do not add".
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS [[
StrCpy $DO_NOT_ADD_TO_PATH 1
StrCpy $ADD_TO_PATH_ALL_USERS 0
StrCpy $ADD_TO_PATH_CURRENT_USER 0
]])
    # Preserve CPACK variable text exactly in generated config.
    set(CPACK_VERBATIM_VARIABLES ON)

    if(NSIS_MAKENSIS_EXECUTABLE)
        set(CPACK_NSIS_EXECUTABLE "${NSIS_MAKENSIS_EXECUTABLE}")
    else()
        find_program(CPACK_NSIS_EXECUTABLE NAMES makensis)
    endif()

    include(CPack)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_target(dist_installer
            COMMAND ${CMAKE_COMMAND} -E echo "dist_installer requires Release build type. Reconfigure with CMAKE_BUILD_TYPE=Release."
            VERBATIM
        )
    elseif(NOT CPACK_NSIS_EXECUTABLE OR NOT EXISTS "${CPACK_NSIS_EXECUTABLE}")
        add_custom_target(dist_installer
            COMMAND ${CMAKE_COMMAND} -E echo "NSIS executable not found. Set NSIS_MAKENSIS_EXECUTABLE or ensure makensis is in PATH."
            VERBATIM
        )
    else()
        add_custom_target(dist_installer
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_INSTALLER_DIR}"
            COMMAND "${CMAKE_CPACK_COMMAND}" -G NSIS --config "${CMAKE_BINARY_DIR}/CPackConfig.cmake"
            DEPENDS dist_win
            COMMENT "Build NSIS installer into ${DIST_INSTALLER_DIR}"
            VERBATIM
        )
    endif()
else()
    add_custom_target(dist_win
        COMMAND ${CMAKE_COMMAND} -E echo "dist_win is only available on Windows."
        VERBATIM
    )
    add_custom_target(dist_installer
        COMMAND ${CMAKE_COMMAND} -E echo "dist_installer is only available on Windows."
        VERBATIM
    )
endif()

