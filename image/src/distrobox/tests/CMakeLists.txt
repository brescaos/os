find_package(Qt6 REQUIRED COMPONENTS Test)
set(CMAKE_AUTOMOC ON)

add_executable(terminalcolorstest terminalcolorstest.cpp)
target_link_libraries(terminalcolorstest PRIVATE terminal-colors Qt6::Test)
add_test(NAME terminalcolorstest COMMAND terminalcolorstest)
set_tests_properties(terminalcolorstest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
