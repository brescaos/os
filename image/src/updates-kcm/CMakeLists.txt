# System Settings > BrescaOS Updates (README.md). Built by image/Containerfile's
# updates-kcm-build stage and by the development VM's hot reload.
cmake_minimum_required(VERSION 3.22)

project(updates-kcm LANGUAGES CXX)

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(KF6 6.0 REQUIRED COMPONENTS KCMUtils)

# image/build/compile_translations.sh writes it: kcm_brescaos_updates.json.in
# with its translations.
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated kcm_brescaos_updates.json")
set(metadata "${TRANSLATED_METADATA_DIR}/kcm_brescaos_updates.json")
if(NOT EXISTS "${metadata}")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no kcm_brescaos_updates.json")
endif()

# <Qt plugins>/plasma/kcms/systemsettings/kcm_brescaos_updates.so, with ui/
# compiled in. Its translation domain is its plugin id.
kcmutils_add_qml_kcm(kcm_brescaos_updates SOURCES kcm.cpp DISABLE_DESKTOP_FILE_GENERATION)
# moc finds kcm.cpp's kcm_brescaos_updates.json in the include folders.
target_include_directories(kcm_brescaos_updates PRIVATE "${TRANSLATED_METADATA_DIR}")
target_link_libraries(kcm_brescaos_updates PRIVATE KF6::KCMUtilsQuick)

# Its .desktop, which lets the launcher's search find it. What
# kcmutils_add_qml_kcm would generate, but from the translated metadata: it
# only reads the sources' kcm_brescaos_updates.json.
set(desktop_file "${CMAKE_CURRENT_BINARY_DIR}/kcm_brescaos_updates.desktop")
add_custom_command(OUTPUT "${desktop_file}"
    COMMAND KF6::kcmdesktopfilegenerator "${metadata}" "${desktop_file}"
    DEPENDS "${metadata}")
add_custom_target(kcm_brescaos_updates-desktop ALL DEPENDS "${desktop_file}")
install(FILES "${desktop_file}" DESTINATION ${KDE_INSTALL_APPDIR})
