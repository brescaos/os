# workbox (README.md): its library, its command line, its tray applet and
# their tests (with -DBUILD_TESTING=ON).
cmake_minimum_required(VERSION 3.22)

file(STRINGS VERSION.txt WORKBOX_VERSION LIMIT_COUNT 1)
project(workbox VERSION ${WORKBOX_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The static libraries go into the applet's QML plugin too.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The tests only with -DBUILD_TESTING=ON.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMQmlModule)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml)
find_package(KF6 6.0 REQUIRED COMPONENTS Config CoreAddons I18n)

add_compile_definitions(TRANSLATION_DOMAIN="workbox")

# The applet's metadata.json, with its translations (from metadata.json.in,
# by the repository's translations script).
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated metadata.json")
if(NOT EXISTS "${TRANSLATED_METADATA_DIR}/metadata.json")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no metadata.json")
endif()

# The library (lib/), static: the command line, its tests and, later, an
# applet and an app link it.
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

# The tray applet (applet/): its list, out of the QML module so the tests can
# link it, the QML module com.brescaos.workbox, and the plasmoid, whose icon
# its metadata names as a theme icon too.
add_library(workbox-applet-model STATIC applet/workboxmodel.cpp)
target_include_directories(workbox-applet-model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/applet)
target_link_libraries(workbox-applet-model PUBLIC workbox-engines)
qt_extract_metatypes(workbox-applet-model)

ecm_add_qml_module(workbox-qml URI com.brescaos.workbox GENERATE_PLUGIN_SOURCE)
target_sources(workbox-qml PRIVATE applet/workboxmodelqml.h)
target_link_libraries(workbox-qml PRIVATE workbox-applet-model Qt6::Qml)
ecm_finalize_qml_module(workbox-qml DESTINATION ${KDE_INSTALL_QMLDIR})

install(DIRECTORY applet/package/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/com.brescaos.workbox)
install(FILES ${TRANSLATED_METADATA_DIR}/metadata.json DESTINATION ${KDE_INSTALL_DATADIR}/plasma/plasmoids/com.brescaos.workbox)
install(FILES applet/package/contents/icons/workbox-symbolic.svg
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps
        RENAME com.brescaos.workbox.svg)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
