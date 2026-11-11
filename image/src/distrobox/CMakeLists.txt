# The distroboxes' terminal colors (README.md). Built by image/Containerfile's
# distrobox-build stage, by the development VM's hot reload and, with its
# tests, by make check-distrobox.
cmake_minimum_required(VERSION 3.22)

project(distrobox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The tests only with -DBUILD_TESTING=ON (scripts/check_distrobox.sh): the
# image and the hot reload don't build them.
set(BUILD_TESTING OFF CACHE BOOL "Build the tests")

find_package(Qt6 6.6 REQUIRED COMPONENTS Gui)
find_package(KF6ColorScheme 6.8 REQUIRED)

# Everything but main(), so the tests use the same code as the program.
add_library(terminal-colors STATIC terminalcolors.cpp)
target_include_directories(terminal-colors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(terminal-colors PUBLIC Qt6::Gui)

add_executable(bresca-distrobox-colors main.cpp)
target_link_libraries(bresca-distrobox-colors PRIVATE terminal-colors KF6::ColorScheme)
install(TARGETS bresca-distrobox-colors DESTINATION libexec/bresca/distrobox)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
