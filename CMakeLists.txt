cmake_minimum_required(VERSION 3.16)
project(mySwitcher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CORE_SOURCES
    src/core/Config.cpp
    src/core/WindowModel.cpp
    src/core/I18n.cpp
)

set(APP_SOURCES
    src/app/Application.cpp
    src/app/HotkeyManager.cpp
    src/app/SingleInstance.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/SwitcherPanel.cpp
    src/ui/WindowCard.cpp
    src/ui/SettingsDialog.cpp
    src/ui/HotkeyEdit.cpp
)

set(PLATFORM_COMMON
    src/platform/PlatformFactory.cpp
)

if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows/WinWindowSource.cpp
        src/platform/windows/WinExplorerPaths.cpp
        src/app/HotkeyManager_win.cpp
    )
elseif(APPLE)
    enable_language(OBJCXX)
    set(PLATFORM_SOURCES
        src/platform/macos/MacWindowSource.mm
        src/app/HotkeyManager_mac.mm
    )
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
elseif(UNIX)
    find_package(X11)
    if(X11_FOUND)
        set(PLATFORM_SOURCES
            src/platform/linux/LinuxWindowSource.cpp
            src/app/HotkeyManager_linux.cpp
        )
        set(LINUX_X11_LIBS ${X11_LIBRARIES})
        set(LINUX_X11_INCLUDES ${X11_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "Linux build requires X11 development libraries.")
    endif()
endif()

qt_add_executable(mySwitcher
    src/main.cpp
    ${CORE_SOURCES}
    ${APP_SOURCES}
    ${UI_SOURCES}
    ${PLATFORM_COMMON}
    ${PLATFORM_SOURCES}
    resources/resources.qrc
)

target_include_directories(mySwitcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LINUX_X11_INCLUDES}
)

target_link_libraries(mySwitcher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${LINUX_X11_LIBS}
)

if(WIN32)
    target_link_libraries(mySwitcher PRIVATE
        user32 gdi32 dwmapi ole32 oleaut32 shell32 psapi
    )
elseif(APPLE)
    target_link_libraries(mySwitcher PRIVATE
        ${COCOA_FRAMEWORK}
        ${CORE_GRAPHICS_FRAMEWORK}
        ${CORE_FOUNDATION_FRAMEWORK}
        ${APPLICATION_SERVICES_FRAMEWORK}
    )
endif()

if(WIN32)
    set_target_properties(mySwitcher PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS mySwitcher RUNTIME DESTINATION bin)
