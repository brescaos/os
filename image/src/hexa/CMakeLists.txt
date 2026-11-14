# Hexa's QML module (README.md): what its top bar needs to know about the
# wallpaper behind it. Built by the image's Containerfile (its hexa-build
# stage), by the development VM's hot reload and, with its tests, by
# make check-hexa. Everything else in Hexa is copied as it is.
cmake_minimum_required(VERSION 3.22)

project(hexa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The static library goes into the QML module's plugin.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The tests only with -DBUILD_TESTING=ON (make check-hexa): the image and the
# hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMQmlModule)

find_package(Qt6 6.6 REQUIRED COMPONENTS Gui Qml Quick)
find_package(Plasma 6.0 REQUIRED)

# Out of the QML module, without Plasma, so the tests can link it.
add_library(wallpaper-luminance STATIC wallpapercontrast/luminance.cpp)
target_include_directories(wallpaper-luminance PUBLIC wallpapercontrast)
target_link_libraries(wallpaper-luminance PUBLIC Qt6::Gui)

ecm_add_qml_module(hexa-qml URI com.brescaos.hexa GENERATE_PLUGIN_SOURCE)
target_sources(hexa-qml PRIVATE wallpapercontrast/wallpapercontrast.cpp wallpapercontrast/wallpapercontrast.h)
target_link_libraries(hexa-qml PRIVATE wallpaper-luminance Qt6::Qml Qt6::Quick Plasma::Plasma)
ecm_finalize_qml_module(hexa-qml DESTINATION ${KDE_INSTALL_QMLDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
