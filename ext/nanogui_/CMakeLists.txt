

set(exe_name "nanogui")

#################################
##      make the library       ##
#################################
set(NANOGUI_BUILD_PYTHON NO)
set(NANOGUI_BUILD_PYTHON NANOGUI_BUILD_EXAMPLE)

add_subdirectory(nanogui)

#################################
##    sort out include files   ##
#################################

# set(${exe_name}_INCLUDE_DIRS
#     ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/eigen
#     ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/nanovg/src
#     ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glad/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/nanogui/ext/glfw/include
# )
#
# set_property(GLOBAL PROPERTY ${exe_name}_INCLUDE_DIRS
#     ${${exe_name}_INCLUDE_DIRS}
# )
