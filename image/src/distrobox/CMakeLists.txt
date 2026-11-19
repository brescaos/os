# The distroboxes' terminal colors, the tray applet's QML module and the
# KRunner plugin (README.md). Built by image/Containerfile's distrobox-build stage, by the
# development VM's hot reload and, with its tests, by make check-distrobox.
# The applet's QML (applet/) is copied as it is.
cmake_minimum_required(VERSION 3.22)

project(distrobox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The static libraries go into the QML module's plugin.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The tests only with -DBUILD_TESTING=ON (scripts/check_distrobox.sh): the
# image and the hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMQmlModule)

find_package(Qt6 6.6 REQUIRED COMPONENTS Gui Qml Widgets)
find_package(KF6ColorScheme 6.8 REQUIRED)
find_package(KF6 6.0 REQUIRED COMPONENTS CoreAddons I18n JobWidgets Runner WidgetsAddons)

# image/build/compile_translations.sh writes it: krunner/krunner_distrobox.json.in
# with its translations.
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated krunner_distrobox.json")
if(NOT EXISTS "${TRANSLATED_METADATA_DIR}/krunner_distrobox.json")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no krunner_distrobox.json")
endif()

# Everything but main(), so the tests use the same code as the program.
add_library(terminal-colors STATIC terminalcolors.cpp)
target_include_directories(terminal-colors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(terminal-colors PUBLIC Qt6::Gui)

add_executable(bresca-distrobox-colors main.cpp)
target_link_libraries(bresca-distrobox-colors PRIVATE terminal-colors KF6::ColorScheme)
install(TARGETS bresca-distrobox-colors DESTINATION libexec/bresca/distrobox)

# The podman and distrobox commands, for the applet's list and the KRunner
# plugin.
add_library(distrobox-commands STATIC distroboxcommands.cpp)
target_include_directories(distrobox-commands PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(distrobox-commands PUBLIC Qt6::Core)

# The applet's list, out of the QML module so the tests can link it.
add_library(distrobox-core STATIC distroboxmodel.cpp)
target_link_libraries(distrobox-core PUBLIC distrobox-commands terminal-colors Qt6::Gui)
qt_extract_metatypes(distrobox-core)

ecm_add_qml_module(distrobox-qml URI com.brescaos.distrobox GENERATE_PLUGIN_SOURCE)
target_sources(distrobox-qml PRIVATE distroboxmodelqml.h)
target_link_libraries(distrobox-qml PRIVATE distrobox-core Qt6::Qml)
ecm_finalize_qml_module(distrobox-qml DESTINATION ${KDE_INSTALL_QMLDIR})

# The KRunner plugin's job, out of the plugin so the tests can link it.
add_library(distrobox-job STATIC krunner/distroboxjob.cpp)
target_include_directories(distrobox-job PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/krunner)
target_link_libraries(distrobox-job PUBLIC distrobox-commands KF6::CoreAddons)

# <Qt plugins>/kf6/krunner/krunner_distrobox.so, where KRunner looks. Its
# strings are in the applet's domain: one per component (docs/translations.md).
kcoreaddons_add_plugin(krunner_distrobox SOURCES krunner/runner.cpp INSTALL_NAMESPACE "kf6/krunner")
# moc finds runner.cpp's krunner_distrobox.json in the include folders.
target_include_directories(krunner_distrobox PRIVATE "${TRANSLATED_METADATA_DIR}")
target_compile_definitions(krunner_distrobox PRIVATE TRANSLATION_DOMAIN="plasma_applet_com.brescaos.distrobox")
target_link_libraries(krunner_distrobox PRIVATE distrobox-job KF6::I18n KF6::JobWidgets KF6::Runner KF6::WidgetsAddons Qt6::Widgets)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
