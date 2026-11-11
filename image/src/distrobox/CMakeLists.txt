# The distroboxes' terminal colors and the tray applet's QML module
# (README.md). Built by image/Containerfile's distrobox-build stage, by the
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

find_package(Qt6 6.6 REQUIRED COMPONENTS Gui Qml)
find_package(KF6ColorScheme 6.8 REQUIRED)

# Everything but main(), so the tests use the same code as the program.
add_library(terminal-colors STATIC terminalcolors.cpp)
target_include_directories(terminal-colors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(terminal-colors PUBLIC Qt6::Gui)

add_executable(bresca-distrobox-colors main.cpp)
target_link_libraries(bresca-distrobox-colors PRIVATE terminal-colors KF6::ColorScheme)
install(TARGETS bresca-distrobox-colors DESTINATION libexec/bresca/distrobox)

# The applet's list, out of the QML module so the tests can link it.
add_library(distrobox-core STATIC distroboxmodel.cpp)
target_link_libraries(distrobox-core PUBLIC terminal-colors Qt6::Gui)
qt_extract_metatypes(distrobox-core)

ecm_add_qml_module(distrobox-qml URI com.brescaos.distrobox GENERATE_PLUGIN_SOURCE)
target_sources(distrobox-qml PRIVATE distroboxmodelqml.h)
target_link_libraries(distrobox-qml PRIVATE distrobox-core Qt6::Qml)
ecm_finalize_qml_module(distrobox-qml DESTINATION ${KDE_INSTALL_QMLDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
