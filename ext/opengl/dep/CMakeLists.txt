
set(exe_name "OpenGL-Dep")

##################################
#       Gather Dependencies      #
##################################

# if (DR_USE_GL_4_1)
#     get_property(OpenGL_4_1-Dep_INCLUDE_DIRS GLOBAL PROPERTY OpenGL_4_1-Dep_INCLUDE_DIRS)
#     set(LIBRARY
#         OpenGL_4_1-Dep
#     )
#     set(INCLUDE
#         ${OpenGL_4_1-Dep_INCLUDE_DIRS}
#     )
# elseif(DR_USE_GL_2_1)

get_property(OpenGL_2_1-Dep_INCLUDE_DIRS GLOBAL PROPERTY OpenGL_2_1-Dep_INCLUDE_DIRS)
set(LIBRARY
    OpenGL_2_1-Dep
)
set(INCLUDE
    ${OpenGL_2_1-Dep_INCLUDE_DIRS}
)
# endif()

##################################
#         Gather Source          #
##################################

FILE(GLOB_RECURSE source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
)
FILE(GLOB_RECURSE header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
)

add_library(${exe_name} ${source_files} ${header_files})
target_link_libraries(${exe_name}
    ${LIBRARY}
)

##################################
#        sort out headers        #
##################################

set(${exe_name}_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${INCLUDE}
)

set_property(GLOBAL PROPERTY ${exe_name}_INCLUDE_DIRS
    ${${exe_name}_INCLUDE_DIRS}
)
include_directories(
    ${${exe_name}_INCLUDE_DIRS}
)
