# Hexa's compiled parts (README.md): its QML module, what its top bar needs
# to know about the wallpaper behind it, its window decoration and its
# QtWidgets style. Built by
# the image's Containerfile (its hexa-build stage), by the development VM's
# hot reload and, with its tests, by make check-hexa. Everything else in Hexa
# is copied as it is.
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

find_package(Qt6 6.6 REQUIRED COMPONENTS Gui Qml Quick Widgets)
find_package(Plasma 6.0 REQUIRED)
find_package(KF6 6.0 REQUIRED COMPONENTS CoreAddons)
find_package(KDecoration3 6.3 REQUIRED)

# image/build/compile_translations.sh writes it: the decoration's
# com.brescaos.hexa.decoration.json.in with its translations.
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated com.brescaos.hexa.decoration.json")
if(NOT EXISTS "${TRANSLATED_METADATA_DIR}/com.brescaos.hexa.decoration.json")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no com.brescaos.hexa.decoration.json")
endif()

# Out of the QML module, without Plasma, so the tests can link it.
add_library(wallpaper-luminance STATIC wallpapercontrast/luminance.cpp)
target_include_directories(wallpaper-luminance PUBLIC wallpapercontrast)
target_link_libraries(wallpaper-luminance PUBLIC Qt6::Gui)

ecm_add_qml_module(hexa-qml URI com.brescaos.hexa GENERATE_PLUGIN_SOURCE)
target_sources(hexa-qml PRIVATE wallpapercontrast/wallpapercontrast.cpp wallpapercontrast/wallpapercontrast.h)
target_link_libraries(hexa-qml PRIVATE wallpaper-luminance Qt6::Qml Qt6::Quick Plasma::Plasma)
ecm_finalize_qml_module(hexa-qml DESTINATION ${KDE_INSTALL_QMLDIR})

# <Qt plugins>/org.kde.kdecoration3/com.brescaos.hexa.decoration.so: KWin
# finds a decoration there by its file name, its id in kwinrc.
kcoreaddons_add_plugin(com.brescaos.hexa.decoration SOURCES decoration/decoration.cpp decoration/button.cpp
    INSTALL_NAMESPACE "${KDECORATION_PLUGIN_DIR}")
# moc finds decoration.cpp's com.brescaos.hexa.decoration.json in the include folders.
target_include_directories(com.brescaos.hexa.decoration PRIVATE "${TRANSLATED_METADATA_DIR}")
target_link_libraries(com.brescaos.hexa.decoration PRIVATE KDecoration3::KDecoration KF6::CoreAddons Qt6::Gui)

# Out of the plugin, so the tests can link it.
add_library(widget-style STATIC widgetstyle/hexastyle.cpp widgetstyle/hexastyle.h
    widgetstyle/widgets/scrollbar.cpp widgetstyle/widgets/scrollbar.h)
target_include_directories(widget-style PUBLIC widgetstyle)
target_link_libraries(widget-style PUBLIC Qt6::Widgets)

# <Qt plugins>/styles/hexastyle.so, the style Hexa.
add_library(hexastyle MODULE widgetstyle/hexastyleplugin.cpp)
target_link_libraries(hexastyle PRIVATE widget-style)
install(TARGETS hexastyle DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/styles)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
