find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

ecm_add_test(luminancetest.cpp LINK_LIBRARIES wallpaper-luminance Qt6::Test)
ecm_add_test(scrollbarlinetest.cpp LINK_LIBRARIES widget-style Qt6::Test)

# No display needed.
set_tests_properties(luminancetest scrollbarlinetest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
