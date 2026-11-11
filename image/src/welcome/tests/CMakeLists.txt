find_package(Qt6 REQUIRED COMPONENTS Test QuickTest)
include(ECMAddTests)

# The program itself, run as at login and from the menu, with its own state.
ecm_add_test(logintest.cpp LINK_LIBRARIES welcome-core Qt6::Test)
target_compile_definitions(logintest PRIVATE WELCOME_BINARY="$<TARGET_FILE:bresca-welcome>")
add_dependencies(logintest bresca-welcome)

# The QML pages, loaded and driven (Qt Quick Test, qml/tst_*.qml).
add_executable(qmltest qmltest.cpp)
target_link_libraries(qmltest PRIVATE welcome-core welcome-coreplugin Qt6::QuickTest)
target_compile_definitions(qmltest PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
add_test(NAME qmltest COMMAND qmltest)

# No display or session bus needed, and English whatever the developer's locale.
set_tests_properties(logintest qmltest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DBUS_SESSION_BUS_ADDRESS=disabled:;LANGUAGE=en")
