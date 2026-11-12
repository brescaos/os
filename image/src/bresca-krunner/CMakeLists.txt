# KRunner plugin for BrescaOS (README.md). Built by image/Containerfile's
# bresca-krunner-build stage, by the development VM's hot reload and, with its tests,
# by make check-bresca-krunner.
cmake_minimum_required(VERSION 3.22)

project(bresca-krunner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The tests only with -DBUILD_TESTING=ON (scripts/check_bresca_krunner.sh): the image
# and the hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core)
find_package(KF6 6.0 REQUIRED COMPONENTS CoreAddons I18n Runner)

# image/build/compile_translations.sh writes it: krunner_brescaos.json.in with
# its translations.
set(TRANSLATED_METADATA_DIR "" CACHE PATH "Folder with the translated krunner_brescaos.json")
if(NOT EXISTS "${TRANSLATED_METADATA_DIR}/krunner_brescaos.json")
    message(FATAL_ERROR "TRANSLATED_METADATA_DIR has no krunner_brescaos.json")
endif()

add_compile_definitions(TRANSLATION_DOMAIN="krunner_brescaos")

# <Qt plugins>/kf6/krunner/krunner_brescaos.so, where KRunner looks.
kcoreaddons_add_plugin(krunner_brescaos SOURCES runner.cpp INSTALL_NAMESPACE "kf6/krunner")
# moc finds runner.cpp's krunner_brescaos.json in the include folders.
target_include_directories(krunner_brescaos PRIVATE "${TRANSLATED_METADATA_DIR}")
target_link_libraries(krunner_brescaos PRIVATE KF6::I18n KF6::Runner)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
