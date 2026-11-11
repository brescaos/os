find_package(Qt6 REQUIRED COMPONENTS Test QuickTest)
include(ECMAddTests)

# Autotiling's state, on a temporary kwinrc.
ecm_add_test(autotilelayouttest.cpp LINK_LIBRARIES autotile-core Qt6::Test)

# The script, as the build puts it together, in Qt's JavaScript engine, KWin's
# own, with a stub of KWin's API (Qt Quick Test, qml/tst_*.qml).
add_executable(scripttest scripttest.cpp)
target_link_libraries(scripttest PRIVATE autotile-core Qt6::Qml Qt6::QuickTest)
target_compile_definitions(scripttest PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml"
    SCRIPT_FILE="${CMAKE_BINARY_DIR}/main.js")
add_dependencies(scripttest script)
add_test(NAME scripttest COMMAND scripttest)

# No display or session bus needed, and English whatever the developer's locale.
set_tests_properties(autotilelayouttest scripttest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DBUS_SESSION_BUS_ADDRESS=disabled:;LANGUAGE=en")
