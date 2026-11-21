# workbox (README.md): its library, its command line and their tests (with
# -DBUILD_TESTING=ON).
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

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui)
find_package(KF6 6.0 REQUIRED COMPONENTS Config CoreAddons I18n)

add_compile_definitions(TRANSLATION_DOMAIN="workbox")

# The library (lib/), static: the command line, its tests and, later, an
# applet and an app link it.
# What no engine knows: the model, the settings, the Engine interface and the
# commands (lib/core/).
add_library(workbox-core STATIC
            lib/core/color.cpp
            lib/core/commands.cpp
            lib/core/enginejob.h
            lib/core/settings.cpp
            lib/core/terminalcolors.cpp
            lib/core/workbox.cpp)
target_include_directories(workbox-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/core)
target_link_libraries(workbox-core PUBLIC Qt6::Core Qt6::Gui KF6::CoreAddons PRIVATE KF6::ConfigCore KF6::I18n)

# The Engine on Podman (lib/engines/podman/).
add_library(workbox-podman STATIC lib/engines/podman/podmanengine.cpp lib/engines/podman/podmanjob.cpp)
target_include_directories(workbox-podman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/engines/podman)
target_link_libraries(workbox-podman PUBLIC workbox-core PRIVATE KF6::I18n)

# workbox's own settings, under a distribution's (as Distrobox's, in
# <datadir>/defaults/) and the user's.
target_compile_definitions(workbox-core
                           PRIVATE SETTINGS_FILE="${KDE_INSTALL_FULL_DATADIR}/workbox/workbox.conf"
                                   DISTRIBUTION_SETTINGS_FILE="${KDE_INSTALL_FULL_DATADIR}/defaults/workbox/workbox.conf")
install(FILES config/workbox.conf DESTINATION ${KDE_INSTALL_DATADIR}/workbox)

# The command line (cli/) and its bash completion.
add_executable(workbox cli/main.cpp)
target_compile_definitions(workbox PRIVATE WORKBOX_VERSION="${PROJECT_VERSION}")
target_link_libraries(workbox PRIVATE workbox-core workbox-podman KF6::I18n)
install(TARGETS workbox DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES cli/completion.bash DESTINATION ${KDE_INSTALL_DATADIR}/bash-completion/completions RENAME workbox)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
