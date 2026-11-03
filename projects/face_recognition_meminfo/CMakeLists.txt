# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/project.cmake)

# Only pull in the solution component this project actually uses.
#
# armino compiles every component it is told about, regardless of the app's
# PRIV_REQUIRES, so scanning the whole components/ dir drags in dashboard/BT/UI
# code that needs per-project headers and bluetooth config this project does not
# have (network_provisioning needs CONFIG_BLE for dm_gatt.h, display_ui needs a
# panel header, dashcam needs ap/dashcam/...). The sibling projects scan the whole
# dir because they ship those configs; sd_test, which needs none of it, sets no
# EXTRA_COMPONENTS_DIRS at all.
set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/avdk_nn_module
)

project(app)
