
set(exe_name "DruidOpenGL-Dep")

##################################
#       Gather Dependencies      #
##################################

# if (DR_USE_GL_4_1)
#     get_property(DruidOpenGL_4_1-Dep_INCLUDE_DIRS GLOBAL PROPERTY DruidOpenGL_4_1-Dep_INCLUDE_DIRS)
#     set(LIBRARY
#         DruidOpenGL_4_1-Dep
#     )
#     set(INCLUDE
#         ${DruidOpenGL_4_1-Dep_INCLUDE_DIRS}
#     )
# elseif(DR_USE_GL_2_1)

get_property(DruidOpenGL_2_1-Dep_INCLUDE_DIRS GLOBAL PROPERTY DruidOpenGL_2_1-Dep_INCLUDE_DIRS)
set(LIBRARY
    DruidOpenGL_2_1-Dep
)
set(INCLUDE
    ${DruidOpenGL_2_1-Dep_INCLUDE_DIRS}
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

get_property(X_DRUID_INCLUDE_DIRECTORIES GLOBAL PROPERTY X_DRUID_INCLUDE_DIRECTORIES)
list(APPEND X_DRUID_INCLUDE_DIRECTORIES
    ${${exe_name}_INCLUDE_DIRS}
)

set_property(GLOBAL PROPERTY X_DRUID_INCLUDE_DIRECTORIES
    ${X_DRUID_INCLUDE_DIRECTORIES}
)
