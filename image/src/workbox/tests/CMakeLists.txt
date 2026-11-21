find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

# Each test is where its source is, under tests/: lib/core/colortest.cpp
# tests lib/core/color.cpp.
ecm_add_tests(lib/core/workboxtest.cpp
              lib/core/colortest.cpp
              lib/core/settingstest.cpp
              lib/core/commandstest.cpp
              lib/core/terminalcolorstest.cpp
              LINK_LIBRARIES workbox-core Qt6::Test)

# PodmanEngine's commands, with the fake podman of fakes/.
ecm_add_test(lib/engines/podman/podmanenginetest.cpp LINK_LIBRARIES workbox-engines Qt6::Test)
target_compile_definitions(podmanenginetest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# The applet's list, with the fake podman.
ecm_add_test(applet/workboxmodeltest.cpp LINK_LIBRARIES workbox-applet-model Qt6::Test)
target_compile_definitions(workboxmodeltest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# English strings, whatever the developer's locale.
set_tests_properties(workboxtest colortest settingstest commandstest terminalcolorstest podmanenginetest workboxmodeltest
                     PROPERTIES ENVIRONMENT "LANGUAGE=en")
