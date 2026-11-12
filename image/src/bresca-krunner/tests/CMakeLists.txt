find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

# What the queries find, with the plugin loaded as KRunner loads it.
ecm_add_test(runnertest.cpp LINK_LIBRARIES Qt6::Test KF6::Runner)
krunner_configure_test(runnertest krunner_brescaos)

# No display needed; English strings, whatever the developer's locale.
set_tests_properties(runnertest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LANGUAGE=en")
