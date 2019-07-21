
set(exe_name "OpenGL_4_1-Dep")


##################################
#       Gather Dependencies      #
##################################

FIND_PACKAGE(OpenGL REQUIRED)

set(INCLUDE ${OPENGL_INCLUDE_DIR})
set(LIBRARY ${OPENGL_LIBRARIES})

if (${OPENGL_FOUND})
    #message("++ OpenGL")
    #message("    include: " ${OPENGL_INCLUDE_DIR})
    #message("       libs: " ${OPENGL_LIBRARIES})
else(${OPENGL_FOUND})
    message(FATAL_ERROR "## Not Found - OpenGL4.1")
endif (${OPENGL_FOUND})


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
