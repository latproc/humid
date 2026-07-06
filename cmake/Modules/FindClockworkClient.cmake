# CMake file for the clockwork client

find_library(
	ClockworkClient_LIBRARY
	PATHS 
        ${CMAKE_BINARY_DIR}/clockwork_build
        ${CMAKE_BINARY_DIR}/clockwork_stage/lib
        ${PROJECT_SOURCE_DIR}/clockwork/iod/stage/lib
	NAMES cw_client Clockwork
)

find_path(
	ClockworkClient_INCLUDE_DIR
	ClientInterface.h
	PATHS
        ${PROJECT_SOURCE_DIR}/clockwork/iod/src
        /opt/latproc/iod/stage
        /opt/latproc/iod/src
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ClockworkClient 
	DEFAULT_MSG
	ClockworkClient_LIBRARY
	ClockworkClient_INCLUDE_DIR
)

if (ClockworkClient_FOUND)
	set(ClockworkClient_LIBRARIES ${ClockworkClient_LIBRARY})
endif()

mark_as_advanced(ClockworkClient_LIBRARY ClockworkClient_INCLUDE_DIR )
