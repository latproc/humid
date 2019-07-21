
set(exe_name "OpenGL_2_1-Dep")

if(${CMAKE_VERSION} VERSION_GREATER "3.10.0")
    cmake_policy(SET CMP0072 NEW)
endif()

##################################
#       Gather Dependencies      #
##################################

if (MINGW_BUILD)
    set(LIBRARY
        # glew32
        opengl32
    )
else()
    FIND_PACKAGE(OpenGL REQUIRED)

    set(INCLUDE ${OPENGL_INCLUDE_DIR})
    set(LIBRARY ${OPENGL_LIBRARIES})

    if (${OPENGL_FOUND})
        #message("++ OpenGL")
        #message("    include: " ${OPENGL_INCLUDE_DIR})
        #message("       libs: " ${OPENGL_LIBRARIES})
    else(${OPENGL_FOUND})
        message(FATAL_ERROR "## Not Found - OpenGL2.1")
    endif (${OPENGL_FOUND})
endif()

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
