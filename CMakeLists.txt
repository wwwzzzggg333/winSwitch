cmake_minimum_required(VERSION 3.27)
project(winSwitch VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.3 REQUIRED COMPONENTS Core Gui Widgets Test)
include(CTest)

set(CORE_SOURCES
    src/core/Config.cpp
    src/core/ConfigPaths.cpp
    src/core/WindowModel.cpp
    src/core/I18n.cpp
    src/core/AppLog.cpp
)

set(APP_SOURCES
    src/app/Application.cpp
    src/app/HotkeyManager.cpp
    src/app/SingleInstance.cpp
    src/app/StartupManager.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/UiSizing.cpp
    src/ui/SwitcherPanel.cpp
    src/ui/WindowCard.cpp
    src/ui/SettingsDialog.cpp
    src/ui/HotkeyEdit.cpp
    src/ui/AppMessageBox.cpp
)

set(PLATFORM_COMMON
    src/platform/PlatformFactory.cpp
)

if(WIN32)
    set(WINDOW_SOURCE_SOURCES
        src/platform/windows/WinWindowSource.cpp
        src/platform/windows/WinExplorerPaths.cpp
    )
    set(PLATFORM_RESOURCES resources/winswitch.rc)
    set(PLATFORM_SOURCES
        ${WINDOW_SOURCE_SOURCES}
        src/app/HotkeyManager_win.cpp
    )
elseif(APPLE)
    enable_language(OBJCXX)
    set(WINDOW_SOURCE_SOURCES src/platform/macos/MacWindowSource.mm)
    set(PLATFORM_SOURCES
        ${WINDOW_SOURCE_SOURCES}
        src/app/HotkeyManager_mac.mm
    )
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
elseif(UNIX)
    find_package(X11)
    if(X11_FOUND)
        set(WINDOW_SOURCE_SOURCES src/platform/linux/LinuxWindowSource.cpp)
        set(PLATFORM_SOURCES
            ${WINDOW_SOURCE_SOURCES}
            src/app/HotkeyManager_linux.cpp
        )
        set(LINUX_X11_LIBS ${X11_LIBRARIES})
        set(LINUX_X11_INCLUDES ${X11_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Linux build requires X11 development libraries.")
    endif()
endif()

qt_add_executable(winSwitch
    src/main.cpp
    ${CORE_SOURCES}
    ${APP_SOURCES}
    ${UI_SOURCES}
    ${PLATFORM_COMMON}
    ${PLATFORM_SOURCES}
    ${PLATFORM_RESOURCES}
    resources/resources.qrc
)

if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_BIN_DIR}"
    )
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "Could not find windeployqt in the Qt bin directory: ${QT_BIN_DIR}")
    endif()
    add_custom_command(TARGET winSwitch POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --$<IF:$<CONFIG:Debug>,debug,release>
            --no-translations
            --no-opengl-sw
            --no-system-d3d-compiler
            "$<TARGET_FILE:winSwitch>"
        COMMENT "Deploying Qt runtime dependencies beside winSwitch"
        VERBATIM
    )
endif()

target_include_directories(winSwitch PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LINUX_X11_INCLUDES}
)

target_compile_definitions(winSwitch PRIVATE
    WINSWITCH_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(winSwitch PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${LINUX_X11_LIBS}
)

if(WIN32)
    target_link_libraries(winSwitch PRIVATE
        user32 gdi32 dwmapi ole32 oleaut32 shell32 psapi
    )
elseif(APPLE)
    target_link_libraries(winSwitch PRIVATE
        ${COCOA_FRAMEWORK}
        ${CORE_GRAPHICS_FRAMEWORK}
        ${CORE_FOUNDATION_FRAMEWORK}
        ${APPLICATION_SERVICES_FRAMEWORK}
    )
endif()

if(WIN32)
    set_target_properties(winSwitch PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(winSwitch PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.winswitch.app"
        MACOSX_BUNDLE_BUNDLE_NAME "winSwitch"
    )
endif()

install(TARGETS winSwitch
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
install(FILES README.md DESTINATION .)
install(FILES packaging/installed.marker DESTINATION .)
install(PROGRAMS packaging/stop_winswitch.ps1 DESTINATION .)

if(WIN32)
    include(InstallRequiredSystemLibraries)
    qt_generate_deploy_app_script(
        TARGET winSwitch
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        POST_EXCLUDE_REGEXES ".*[\\/]system32[\\/].*\\.dll"
    )
    install(SCRIPT ${deploy_script})
endif()

if(BUILD_TESTING)
    if(WIN32)
        find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
        add_test(
            NAME installer_verification_helpers
            COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -File
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_installer_verification_helpers.ps1"
        )
    endif()

    qt_add_executable(test_config_paths
        tests/test_config_paths.cpp
        src/core/ConfigPaths.cpp
    )
    target_include_directories(test_config_paths PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(test_config_paths PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME config_paths COMMAND test_config_paths)
    set_tests_properties(config_paths PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;$ENV{PATH}"
    )

    qt_add_executable(test_ui_sizing
        tests/test_ui_sizing.cpp
        src/ui/UiSizing.cpp
    )
    target_include_directories(test_ui_sizing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(test_ui_sizing PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME ui_sizing COMMAND test_ui_sizing)
    set_tests_properties(ui_sizing PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;$ENV{PATH}"
    )

    qt_add_executable(test_switcher_panel
        tests/test_switcher_panel.cpp
        src/ui/SwitcherPanel.cpp
        src/ui/WindowCard.cpp
        src/core/WindowModel.cpp
        src/core/I18n.cpp
        src/core/Config.cpp
        src/core/ConfigPaths.cpp
        src/core/AppLog.cpp
    )
    target_include_directories(test_switcher_panel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(test_switcher_panel PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    add_test(NAME switcher_panel COMMAND test_switcher_panel)
    set_tests_properties(switcher_panel PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;$ENV{PATH}"
    )

    qt_add_executable(test_main_window_lifecycle
        tests/test_main_window_lifecycle.cpp
        ${CORE_SOURCES}
        ${UI_SOURCES}
        ${PLATFORM_COMMON}
        ${WINDOW_SOURCE_SOURCES}
    )
    target_include_directories(test_main_window_lifecycle PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LINUX_X11_INCLUDES}
    )
    target_link_libraries(test_main_window_lifecycle PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
        ${LINUX_X11_LIBS}
    )
    if(WIN32)
        target_link_libraries(test_main_window_lifecycle PRIVATE
            user32 gdi32 dwmapi ole32 oleaut32 shell32 psapi
        )
    elseif(APPLE)
        target_link_libraries(test_main_window_lifecycle PRIVATE
            ${COCOA_FRAMEWORK}
            ${CORE_GRAPHICS_FRAMEWORK}
            ${CORE_FOUNDATION_FRAMEWORK}
            ${APPLICATION_SERVICES_FRAMEWORK}
        )
    endif()
    add_test(NAME main_window_lifecycle COMMAND test_main_window_lifecycle)
    set_tests_properties(main_window_lifecycle PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;$ENV{PATH}"
    )
endif()

if(WIN32)
    find_program(INNOSETUP_COMPILER
        NAMES ISCC.exe
        HINTS
            "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
            "$ENV{LOCALAPPDATA}/Programs/Inno Setup 7"
            "$ENV{ProgramFiles}/Inno Setup 6"
            "$ENV{ProgramFiles}/Inno Setup 7"
            "$ENV{ProgramFiles\(x86\)}/Inno Setup 6"
            "$ENV{ProgramFiles\(x86\)}/Inno Setup 7"
    )
    set(CPACK_VERBATIM_VARIABLES YES)
    set(CPACK_GENERATOR "INNOSETUP")
    set(CPACK_PACKAGE_NAME "winSwitch")
    set(CPACK_PACKAGE_VENDOR "winSwitch")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast keyboard-driven window switcher for Windows")
    set(CPACK_PACKAGE_FILE_NAME "winSwitch-${PROJECT_VERSION}-windows-x64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "winSwitch")
    set(CPACK_PACKAGE_EXECUTABLES "winSwitch" "winSwitch")
    set(CPACK_CREATE_DESKTOP_LINKS "winSwitch")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

    set(CPACK_INNOSETUP_INSTALL_ROOT "{localappdata}\\Programs")
    if(INNOSETUP_COMPILER)
        set(CPACK_INNOSETUP_EXECUTABLE "${INNOSETUP_COMPILER}")
    endif()
    set(CPACK_INNOSETUP_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/winswitch.ico")
    set(CPACK_INNOSETUP_USE_MODERN_WIZARD ON)
    set(CPACK_INNOSETUP_IGNORE_LICENSE_PAGE ON)
    set(CPACK_INNOSETUP_IGNORE_README_PAGE OFF)
    set(CPACK_INNOSETUP_RUN_EXECUTABLES "winSwitch")
    set(CPACK_INNOSETUP_EXTRA_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/winswitch.iss")
    set(CPACK_INNOSETUP_SETUP_PrivilegesRequired "lowest")
    set(CPACK_INNOSETUP_SETUP_CloseApplications "force")
    set(CPACK_INNOSETUP_SETUP_RestartApplications OFF)
    set(CPACK_INNOSETUP_SETUP_UninstallDisplayIcon "{app}\\bin\\winSwitch.exe")

    include(CPack)
endif()
