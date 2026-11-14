find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

ecm_add_test(luminancetest.cpp LINK_LIBRARIES wallpaper-luminance Qt6::Test)

# No display needed.
set_tests_properties(luminancetest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
