find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

ecm_add_test(terminalcolorstest.cpp LINK_LIBRARIES terminal-colors Qt6::Test)

# The applet's list and its actions, with the fake podman and distrobox of
# fakes/.
ecm_add_test(distroboxmodeltest.cpp LINK_LIBRARIES distrobox-core Qt6::Test)
target_compile_definitions(distroboxmodeltest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# No display needed.
set_tests_properties(terminalcolorstest distroboxmodeltest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
