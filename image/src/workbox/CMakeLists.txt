# workbox's command line (README.md). Built by image/Containerfile's
# workbox-build stage, by the development VM's hot reload and, with its tests,
# by make check-workbox.
cmake_minimum_required(VERSION 3.22)

project(workbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The tests only with -DBUILD_TESTING=ON (scripts/check_workbox.sh): the image
# and the hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui)
find_package(KF6 6.0 REQUIRED COMPONENTS Config I18n)

add_compile_definitions(TRANSLATION_DOMAIN="workbox")

# What no engine knows: the model, the settings, the Engine interface and the
# commands (core/), out of the program so the tests and, later, an app use it.
add_library(workbox-core STATIC core/color.cpp core/commands.cpp core/settings.cpp core/workbox.cpp)
target_include_directories(workbox-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core)
target_link_libraries(workbox-core PUBLIC Qt6::Core Qt6::Gui PRIVATE KF6::ConfigCore KF6::I18n)

# The Engine on Podman (engines/podman/).
add_library(workbox-podman STATIC engines/podman/podmanengine.cpp)
target_include_directories(workbox-podman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engines/podman)
target_link_libraries(workbox-podman PUBLIC workbox-core PRIVATE KF6::I18n)

# The command line (cli/).
add_executable(workbox cli/main.cpp)
target_link_libraries(workbox PRIVATE workbox-core workbox-podman KF6::I18n)
install(TARGETS workbox DESTINATION ${KDE_INSTALL_BINDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
