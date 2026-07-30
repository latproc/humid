# Find the Clockwork client library used by Humid.
#
# Prefer the humid-bundled clockwork submodule (stage after
# `cd clockwork/iod && make client-install`). Do not silently pick an
# unrelated tree such as /opt/latproc: mismatched headers vs libcw_client.a
# cause compile errors (e.g. missing addSetupResponder) or runtime crashes.

set(_ClockworkClient_LIB_HINT "${PROJECT_SOURCE_DIR}/clockwork/iod/stage/lib")
set(_ClockworkClient_INC_HINT "${PROJECT_SOURCE_DIR}/clockwork/iod/src")

# If the submodule client is already staged, force it over any cached path
# from a previous configure (common on panels that once used /opt/latproc).
if (EXISTS "${_ClockworkClient_LIB_HINT}/libcw_client.a")
  set(ClockworkClient_LIBRARY "${_ClockworkClient_LIB_HINT}/libcw_client.a"
      CACHE FILEPATH "Clockwork client library" FORCE)
endif()
if (EXISTS "${_ClockworkClient_INC_HINT}/ClientInterface.h")
  set(ClockworkClient_INCLUDE_DIR "${_ClockworkClient_INC_HINT}"
      CACHE PATH "Clockwork client headers" FORCE)
endif()

if (NOT ClockworkClient_LIBRARY)
  find_library(
    ClockworkClient_LIBRARY
    NAMES cw_client Clockwork
    PATHS ${_ClockworkClient_LIB_HINT}
    NO_DEFAULT_PATH)
endif()

# Legacy fallback only when the submodule client has not been staged yet.
if (NOT ClockworkClient_LIBRARY)
  find_library(
    ClockworkClient_LIBRARY
    NAMES cw_client Clockwork
    PATHS
      /opt/latproc/iod/stage/lib
      /opt/latproc/iod/build/Release
      /opt/latproc/iod/build/Debug
      /opt/latproc/iod/build)
  if (ClockworkClient_LIBRARY)
    message(WARNING
      "Using Clockwork client outside the humid submodule:\n"
      "  ${ClockworkClient_LIBRARY}\n"
      "Prefer: (cd clockwork/iod && make client-install) then reconfigure humid.")
  endif()
endif()

if (NOT ClockworkClient_INCLUDE_DIR)
  find_path(
    ClockworkClient_INCLUDE_DIR
    ClientInterface.h
    PATHS ${_ClockworkClient_INC_HINT}
    NO_DEFAULT_PATH)
endif()

if (NOT ClockworkClient_INCLUDE_DIR)
  find_path(
    ClockworkClient_INCLUDE_DIR
    ClientInterface.h
    PATHS
      /opt/latproc/iod/stage
      /opt/latproc/iod/src)
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ClockworkClient
  DEFAULT_MSG
  ClockworkClient_LIBRARY
  ClockworkClient_INCLUDE_DIR)

if (ClockworkClient_FOUND)
  set(ClockworkClient_LIBRARIES ${ClockworkClient_LIBRARY})
endif()

mark_as_advanced(ClockworkClient_LIBRARY ClockworkClient_INCLUDE_DIR)
