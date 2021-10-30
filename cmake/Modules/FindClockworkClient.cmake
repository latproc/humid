# CMake file for the clockwork client

find_library(
	ClockworkClient_LIBRARY
	PATHS /opt/latproc/iod/stage/lib /opt/latproc/iod/build
	NAMES cw_client Clockwork
)

find_path(
	ClockworkClient_INCLUDE_DIR
	ClientInterface.h
	PATHS /opt/latproc/iod/stage /opt/latproc/iod/src
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
