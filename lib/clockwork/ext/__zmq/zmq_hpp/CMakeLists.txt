
set(exe_name "ZMQ_HPP-Dep")

get_property(ZMQ-Dep_INCLUDE_DIRS GLOBAL PROPERTY ZMQ-Dep_INCLUDE_DIRS)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
# FIND_PACKAGE(ZMQ 4.2 REQUIRED)
set(INCLUDE
    ${ZMQ_internal-Dep_INCLUDE_DIRS}
	# ${ZMQ_INCLUDE_DIRS}
)
set(LIBRARY
    ZMQ_internal-Dep
	# ${ZMQ_LIBRARIES}
)

#################################
##      make the library       ##
#################################

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

#################################
##    sort out include files   ##
#################################
set(${exe_name}_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # ${INCLUDE}
)

set_property(GLOBAL PROPERTY ${exe_name}_INCLUDE_DIRS
    ${${exe_name}_INCLUDE_DIRS}
)
include_directories(
    ${${exe_name}_INCLUDE_DIRS}
    ${INCLUDE}
)
