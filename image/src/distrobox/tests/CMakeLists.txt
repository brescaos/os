find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

ecm_add_test(terminalcolorstest.cpp LINK_LIBRARIES terminal-colors Qt6::Test)

# The applet's list and its actions, with the fake podman and distrobox of
# fakes/.
ecm_add_test(distroboxmodeltest.cpp LINK_LIBRARIES distrobox-core Qt6::Test)
target_compile_definitions(distroboxmodeltest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# The KRunner plugin's job, with the fake distrobox.
ecm_add_test(distroboxjobtest.cpp LINK_LIBRARIES distrobox-job Qt6::Test)
target_compile_definitions(distroboxjobtest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# What the KRunner plugin's queries find, with the plugin loaded as KRunner
# loads it and the fake podman.
ecm_add_test(runnertest.cpp LINK_LIBRARIES Qt6::Test KF6::Runner)
krunner_configure_test(runnertest krunner_distrobox)
target_compile_definitions(runnertest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# No display needed; English strings, whatever the developer's locale.
set_tests_properties(terminalcolorstest distroboxmodeltest distroboxjobtest runnertest
                     PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LANGUAGE=en")
