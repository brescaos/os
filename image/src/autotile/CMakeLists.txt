# Autotiling (README.md): the KWin script (script/), its KCM (kcm.cpp, ui/) and
# the QML module that and the tray applet share (com.brescaos.autotile:
# common/, qml/). The applet (applet/) and the translations aren't compiled:
# image/Containerfile copies them. Built by its autotile-build stage, by the
# development VM's hot reload and, with its tests, by make check-autotile.
cmake_minimum_required(VERSION 3.22)

project(autotile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# autotile-core goes into the QML module and the tests, shared objects.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The tests only with -DBUILD_TESTING=ON (scripts/check_autotile.sh): the image
# and the hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMQmlModule)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core DBus Qml Quick)
find_package(KF6 6.0 REQUIRED COMPONENTS Config CoreAddons I18n KCMUtils)

# image/build/compile_translations.sh writes it: kcm_brescaos_autotile.json.in
# with its translations.
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated kcm_brescaos_autotile.json")
set(metadata "${TRANSLATED_METADATA_DIR}/kcm_brescaos_autotile.json")
if(NOT EXISTS "${metadata}")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no kcm_brescaos_autotile.json")
endif()

# The script: KWin loads one file and scripts can't include others, so the
# layouts and then main.js. Its metadata.json comes with the translations.
file(GLOB layouts CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/script/layouts/*.js)
list(SORT layouts)
set(script_sources ${layouts} ${CMAKE_CURRENT_SOURCE_DIR}/script/main.js)
set(script ${CMAKE_CURRENT_BINARY_DIR}/main.js)
add_custom_command(OUTPUT ${script}
    COMMAND ${CMAKE_COMMAND} -E cat ${script_sources} > ${script}
    DEPENDS ${script_sources})
add_custom_target(script ALL DEPENDS ${script})
install(FILES ${script} DESTINATION ${KDE_INSTALL_DATADIR}/kwin/scripts/com.brescaos.autotile/contents/code)

# Autotiling's state (AutotileLayout), a static library for the tests, with its
# metatypes, so the QML module's qmltypes describe it (QML_FOREIGN).
add_library(autotile-core STATIC common/autotilelayout.cpp)
target_include_directories(autotile-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_definitions(autotile-core PRIVATE TRANSLATION_DOMAIN="kcm_brescaos_autotile")
target_link_libraries(autotile-core PUBLIC Qt6::Core Qt6::DBus KF6::ConfigCore KF6::CoreAddons KF6::I18n)
qt_extract_metatypes(autotile-core)

# The QML module com.brescaos.autotile, for the applet and the KCM: the state,
# and the layouts' list and symbols (qml/).
ecm_add_qml_module(autotile-qml URI com.brescaos.autotile GENERATE_PLUGIN_SOURCE)
target_sources(autotile-qml PRIVATE common/autotilelayoutqml.h)
ecm_target_qml_sources(autotile-qml SOURCES qml/LayoutList.qml qml/LayoutIcon.qml)
# Next to the compiled QML, which finds them relative to itself.
qt_add_resources(autotile-qml "autotile-icons"
    PREFIX "/qt/qml/com/brescaos/autotile/icons"
    BASE qml/icons
    FILES qml/icons/layout-binary-tree-symbolic.svg qml/icons/layout-master-symbolic.svg
        qml/icons/layout-grid-symbolic.svg qml/icons/layout-none-symbolic.svg)
target_link_libraries(autotile-qml PRIVATE autotile-core Qt6::Qml Qt6::Quick)
ecm_finalize_qml_module(autotile-qml DESTINATION ${KDE_INSTALL_QMLDIR})

# <Qt plugins>/plasma/kcms/systemsettings/kcm_brescaos_autotile.so, with ui/
# compiled in. Its translation domain is its plugin id.
kcmutils_add_qml_kcm(kcm_brescaos_autotile SOURCES kcm.cpp DISABLE_DESKTOP_FILE_GENERATION)
# moc finds kcm.cpp's kcm_brescaos_autotile.json in the include folders.
target_include_directories(kcm_brescaos_autotile PRIVATE "${TRANSLATED_METADATA_DIR}")
target_link_libraries(kcm_brescaos_autotile PRIVATE KF6::KCMUtilsQuick)

# Its .desktop, which lets the launcher's search find it, from the translated
# metadata, as in updates-kcm.
set(desktop_file "${CMAKE_CURRENT_BINARY_DIR}/kcm_brescaos_autotile.desktop")
add_custom_command(OUTPUT "${desktop_file}"
    COMMAND KF6::kcmdesktopfilegenerator "${metadata}" "${desktop_file}"
    DEPENDS "${metadata}")
add_custom_target(kcm_brescaos_autotile-desktop ALL DEPENDS "${desktop_file}")
install(FILES "${desktop_file}" DESTINATION ${KDE_INSTALL_APPDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
