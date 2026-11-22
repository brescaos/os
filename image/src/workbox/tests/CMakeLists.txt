find_package(Qt6 REQUIRED COMPONENTS Test)
include(ECMAddTests)

# Each test is where its source is, under tests/: lib/core/colortest.cpp
# tests lib/core/color.cpp.
ecm_add_tests(lib/core/workboxtest.cpp
              lib/core/colortest.cpp
              lib/core/settingstest.cpp
              lib/core/volumestest.cpp
              lib/core/commandstest.cpp
              lib/core/terminalcolorstest.cpp
              LINK_LIBRARIES workbox-core Qt6::Test)

# PodmanEngine's commands, with the fake podman of fakes/.
ecm_add_test(lib/engines/podman/podmanenginetest.cpp LINK_LIBRARIES workbox-engines Qt6::Test)
target_compile_definitions(podmanenginetest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# The app's list, with the fake podman.
ecm_add_test(app/workboxmodeltest.cpp LINK_LIBRARIES workbox-model Qt6::Test)
target_compile_definitions(workboxmodeltest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# The app's window, its dialogs and its tray icon, with the fake podman,
# offscreen.
ecm_add_tests(app/window/mainwindowtest.cpp app/window/trayicontest.cpp app/window/dialogs/workbox/createtest.cpp
              app/window/dialogs/volume/addtest.cpp
              LINK_LIBRARIES workbox-window KF6::StatusNotifierItem Qt6::Test)
target_compile_definitions(mainwindowtest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")
target_compile_definitions(trayicontest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")
target_compile_definitions(createtest PRIVATE FAKES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fakes")

# English strings, whatever the developer's locale.
set_tests_properties(workboxtest colortest settingstest volumestest commandstest terminalcolorstest podmanenginetest workboxmodeltest
                     PROPERTIES ENVIRONMENT "LANGUAGE=en")
set_tests_properties(mainwindowtest trayicontest createtest addtest PROPERTIES ENVIRONMENT "LANGUAGE=en;QT_QPA_PLATFORM=offscreen")
