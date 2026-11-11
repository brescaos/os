# Bresca Welcome (README.md). Built by image/Containerfile's welcome-build
# stage, by the development VM's hot reload and, with its tests, by
# make check-welcome.
cmake_minimum_required(VERSION 3.22)

project(welcome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The tests only with -DBUILD_TESTING=ON (scripts/check_welcome.sh): the image
# and the hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(ECM 6.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt6 6.6 REQUIRED COMPONENTS Quick QuickControls2 Widgets)
find_package(KF6 6.8 REQUIRED COMPONENTS Config I18n)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

# Everything but main(): the revision state and the QML module, compiled in.
# A static library, so the tests use the same code as the program.
qt_add_library(welcome-core STATIC revision.cpp)
qt_add_qml_module(welcome-core
    URI com.brescaos.welcome
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/com/brescaos/welcome
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/StepPage.qml
        qml/WelcomePage.qml
        qml/DonePage.qml
)
target_include_directories(welcome-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(welcome-core PUBLIC TRANSLATION_DOMAIN="bresca-welcome")
target_link_libraries(welcome-core PUBLIC Qt6::Quick KF6::ConfigCore KF6::I18n KF6::I18nQml)

add_executable(bresca-welcome main.cpp)
target_link_libraries(bresca-welcome PRIVATE welcome-core welcome-coreplugin Qt6::QuickControls2 Qt6::Widgets)
install(TARGETS bresca-welcome ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
