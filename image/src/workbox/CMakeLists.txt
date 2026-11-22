# workbox (README.md): its library, its command line, its app (a window and
# its icon in the system tray) and their tests (with -DBUILD_TESTING=ON).
cmake_minimum_required(VERSION 3.22)

file(STRINGS VERSION.txt WORKBOX_VERSION LIMIT_COUNT 1)
project(workbox VERSION ${WORKBOX_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The tests only with -DBUILD_TESTING=ON.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6 6.0 REQUIRED COMPONENTS ColorScheme Config CoreAddons I18n Parts StatusNotifierItem WidgetsAddons)

add_compile_definitions(TRANSLATION_DOMAIN="workbox")

# The app's .desktop, with its translations (from its *.in, by the
# repository's translations script).
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated com.brescaos.workbox.desktop")
if(NOT EXISTS "${TRANSLATED_METADATA_DIR}/com.brescaos.workbox.desktop")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no com.brescaos.workbox.desktop")
endif()

# The library (lib/), static: the command line, the app and their tests link
# it.
# What no engine knows: the model, the settings, the Engine interface and the
# commands (lib/core/).
add_library(workbox-core STATIC
            lib/core/color.cpp
            lib/core/commands.cpp
            lib/core/enginejob.h
            lib/core/settings.cpp
            lib/core/volumes.cpp
            lib/core/terminalcolors.cpp
            lib/core/workbox.cpp)
target_include_directories(workbox-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/core)
target_link_libraries(workbox-core PUBLIC Qt6::Core Qt6::Gui KF6::CoreAddons PRIVATE KF6::ConfigCore KF6::I18n)

# The engines (lib/engines/): the Engine on Podman, and the one workbox.conf
# names.
add_library(workbox-engines STATIC lib/engines/engines.cpp lib/engines/podman/podmanengine.cpp lib/engines/podman/podmanjob.cpp)
target_include_directories(workbox-engines PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/engines ${CMAKE_CURRENT_SOURCE_DIR}/lib/engines/podman)
target_link_libraries(workbox-engines PUBLIC workbox-core PRIVATE KF6::I18n)

# workbox's own settings, under a distribution's (in <datadir>/defaults/)
# and the user's.
target_compile_definitions(workbox-core
                           PRIVATE SETTINGS_FILE="${KDE_INSTALL_FULL_DATADIR}/workbox/workbox.conf"
                                   DISTRIBUTION_SETTINGS_FILE="${KDE_INSTALL_FULL_DATADIR}/defaults/workbox/workbox.conf")
install(FILES config/workbox.conf DESTINATION ${KDE_INSTALL_DATADIR}/workbox)

# The command line (cli/) and its bash completion.
add_executable(workbox cli/main.cpp)
target_compile_definitions(workbox PRIVATE WORKBOX_VERSION="${PROJECT_VERSION}")
target_link_libraries(workbox PRIVATE workbox-engines KF6::I18n)
install(TARGETS workbox DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES cli/completion.bash DESTINATION ${KDE_INSTALL_DATADIR}/bash-completion/completions RENAME workbox)

# The app (app/): its list of workboxes, out of the program so the tests can
# link it; its window, with Konsole's part for its terminals, and its icon in
# the system tray (a StatusNotifierItem, there while the app runs); its
# .desktop and its icons: the app's, and the toolbox the tray and the window
# draw.
add_library(workbox-model STATIC app/workboxmodel.cpp)
target_include_directories(workbox-model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app)
target_link_libraries(workbox-model PUBLIC workbox-engines)

add_library(workbox-window STATIC app/window/mainwindow.cpp app/window/terminals.cpp app/window/trayicon.cpp
                                  app/window/workboxdelegate.cpp app/window/dialogs/formparts.cpp
                                  app/window/dialogs/workbox/create.cpp app/window/dialogs/volume/add.cpp)
target_include_directories(workbox-window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app/window)
target_link_libraries(workbox-window
                      PUBLIC workbox-model Qt6::Widgets
                      PRIVATE KF6::ColorScheme KF6::I18n KF6::Parts KF6::StatusNotifierItem KF6::WidgetsAddons)

add_executable(workbox-app app/window/main.cpp)
target_link_libraries(workbox-app PRIVATE workbox-window KF6::I18n)
install(TARGETS workbox-app DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES ${TRANSLATED_METADATA_DIR}/com.brescaos.workbox.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES app/icons/com.brescaos.workbox.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(FILES app/icons/workbox-symbolic.svg
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps
        RENAME com.brescaos.workbox-symbolic.svg)
install(FILES app/icons/workbox-running-symbolic.svg
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps
        RENAME com.brescaos.workbox-running-symbolic.svg)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
