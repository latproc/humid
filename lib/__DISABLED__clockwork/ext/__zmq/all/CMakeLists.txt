
set(exe_name "ZMQ-Dep")


##################################
#       Gather Dependencies      #
##################################

get_property(ZMQ_internal-Dep_INCLUDE_DIRS GLOBAL PROPERTY ZMQ_internal-Dep_INCLUDE_DIRS)
get_property(ZMQ_HPP-Dep_INCLUDE_DIRS GLOBAL PROPERTY ZMQ_HPP-Dep_INCLUDE_DIRS)

set(INCLUDE
    ${ZMQ_internal-Dep_INCLUDE_DIRS}
    ${ZMQ_HPP-Dep_INCLUDE_DIRS}
)
set(LIBRARY
    ZMQ_internal-Dep
    ZMQ_HPP-Dep
)


##################################
#          Build Binary          #
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
target_include_directories(${exe_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${INCLUDE}
)
