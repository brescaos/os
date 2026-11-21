find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

# Each test is where its source is, under tests/: core/colortest.cpp tests
# core/color.cpp.
ecm_add_tests(core/workboxtest.cpp core/colortest.cpp core/settingstest.cpp core/commandstest.cpp core/terminalcolorstest.cpp LINK_LIBRARIES workbox-core Qt6::Test)

# PodmanEngine's commands, with the fake podman of fakes/.
ecm_add_test(engines/podman/podmanenginetest.cpp LINK_LIBRARIES workbox-podman Qt6::Test)
target_compile_definitions(podmanenginetest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# English strings, whatever the developer's locale.
set_tests_properties(workboxtest colortest settingstest commandstest terminalcolorstest podmanenginetest PROPERTIES ENVIRONMENT "LANGUAGE=en")
