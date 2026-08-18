# 3.5: still present on some older Ubuntu HMI panels (e.g. 16.04 / cmake 3.5.1).
# Prefer 3.10+ where available; avoid features that require newer CMake unless gated.
cmake_minimum_required (VERSION 3.5)
project (Latproc)

set (Latproc_VERSION_MAJOR 0)
set (Latproc_VERSION_MINOR 9.7)
option(USE_ASAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer instrumentation" OFF)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		  set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/stage" CACHE PATH "..." FORCE)
endif()

if (EXISTS "${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
	include("${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()

cmake_policy(SET CMP0008 NEW)
cmake_policy(SET CMP0015 NEW)
if(POLICY CMP0167)
    # Keep FindBoost behavior across development and older production CMake versions.
    cmake_policy(SET CMP0167 OLD)
endif()

function(target_link_directories_compat target dir)
       if(CMAKE_VERSION VERSION_LESS "3.13")
               target_link_libraries(${target} "-L${dir}")
       else()
               target_link_directories(${target} PRIVATE "${dir}")
       endif()
endfunction()

#set (CMAKE_VERBOSE_MAKEFILE false)
set (CMAKE_EXPORT_COMPILE_COMMANDS true)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall ")
set(CLOCKWORK_SANITIZER_FLAGS "")
if (USE_ASAN)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(CLOCKWORK_SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
    else()
        message(FATAL_ERROR "USE_ASAN requires a Clang or GCC toolchain")
    endif()
endif()
if (CLOCKWORK_SANITIZER_FLAGS)
    # add_compile_definitions needs CMake >= 3.12
    if (CMAKE_VERSION VERSION_LESS "3.12")
        add_definitions(-DBOOST_USE_ASAN)
    else()
        add_compile_definitions(BOOST_USE_ASAN)
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLOCKWORK_SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLOCKWORK_SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CLOCKWORK_SANITIZER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CLOCKWORK_SANITIZER_FLAGS}")
    message(STATUS "Sanitizers enabled: ${CLOCKWORK_SANITIZER_FLAGS}")
endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-value")
IF("${CMAKE_SYSTEM}" MATCHES "Linux")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
endif()
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

set (CLOCKWORK_DIR ${PROJECT_SOURCE_DIR}/src/)
set (BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

configure_file (
  "${PROJECT_SOURCE_DIR}/LatprocConfig.h.in"
  "${PROJECT_BINARY_DIR}/LatprocConfig.h"
)

set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")
# Prefer context when present (full iod process/scheduler). Not required for
# cw_client / humid: json path evaluation no longer uses boost::context::fiber.
FIND_PACKAGE(Boost COMPONENTS thread program_options filesystem chrono date_time context)
if (NOT Boost_FOUND)
  FIND_PACKAGE(Boost COMPONENTS thread program_options filesystem chrono date_time REQUIRED)
endif()
#FIND_PACKAGE(Mosquitto)
#FIND_PACKAGE(MODBUS)
FIND_PACKAGE(LibXml2)
FIND_PACKAGE(ZeroMQ REQUIRED)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ${ZeroMQ_INCLUDE_DIR})

set(HEADER_FILES
	${CLOCKWORK_DIR}/cw_client.h
	${CLOCKWORK_DIR}/MessagingInterface.h
	${CLOCKWORK_DIR}/SocketMonitor.h
	${CLOCKWORK_DIR}/ConnectionManager.h
	${CLOCKWORK_DIR}/MessageLog.h
	${CLOCKWORK_DIR}/MessageEncoding.h
	${CLOCKWORK_DIR}/MessageHeader.h
	${CLOCKWORK_DIR}/regular_expressions.h
	${CLOCKWORK_DIR}/Logger.h
	${CLOCKWORK_DIR}/anet.h
	${CLOCKWORK_DIR}/clock.h
	${CLOCKWORK_DIR}/cJSON.h
	${CLOCKWORK_DIR}/DebugExtra.h
	${CLOCKWORK_DIR}/value.h
    ${CLOCKWORK_DIR}/urlencode.h
	${CLOCKWORK_DIR}/symboltable.h
	${CLOCKWORK_DIR}/rate.h
	${CLOCKWORK_DIR}/watchdog.h
	${CLOCKWORK_DIR}/Message.h
	${CLOCKWORK_DIR}/Parameter.h
	${CLOCKWORK_DIR}/Receiver.h
    ${CLOCKWORK_DIR}/json_expr_parser.h
    ${CLOCKWORK_DIR}/json_expression.h
    ${CLOCKWORK_DIR}/ThreadSafeQueue.h
    ${CLOCKWORK_DIR}/ThreadSafeList.h
    )

# process.cpp uses boost::context::fiber (scheduler helpers) — keep it out of
# cw_client so humid panels on Bionic Boost 1.65 can build the client.
add_library (cw_client
	${CLOCKWORK_DIR}/MessagingInterface.cpp
	${CLOCKWORK_DIR}/MessageLog.cpp
	${CLOCKWORK_DIR}/MessageEncoding.cpp
	${CLOCKWORK_DIR}/MessageHeader.cpp
	${CLOCKWORK_DIR}/regular_expressions.cpp
	${CLOCKWORK_DIR}/SocketMonitor.cpp
	${CLOCKWORK_DIR}/ConnectionManager.cpp
	${CLOCKWORK_DIR}/Logger.cpp
	${CLOCKWORK_DIR}/anet.c
	${CLOCKWORK_DIR}/clock.cpp
	${CLOCKWORK_DIR}/cJSON.c
	${CLOCKWORK_DIR}/DebugExtra.cpp
	${CLOCKWORK_DIR}/value.cpp
	${CLOCKWORK_DIR}/symboltable.cpp
	${CLOCKWORK_DIR}/rate.cpp
	${CLOCKWORK_DIR}/Message.cpp
	${HEADER_FILES}
)

if (EXISTS "/usr/local/include")
include_directories("/usr/local/include")
endif()
if (EXISTS "/opt/local/include")
include_directories("/opt/local/include")
endif()
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_BINARY_DIR}")
include_directories("${CLOCKWORK_DIR}")

if (EXISTS "/usr/local/lib")
LINK_DIRECTORIES("/usr/local/lib")
endif()
if (EXISTS "/opt/local/lib")
LINK_DIRECTORIES("/opt/local/lib")
endif()

if (RUN_TESTS)
enable_testing()
add_subdirectory(tests)
endif()

install(TARGETS cw_client ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/stage/lib COMPONENT clientlib)

if (CMAKE_VERSION VERSION_LESS "3.15")
    add_custom_target(install_client
      COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_CONFIG_NAME=$<CONFIG> -DCOMPONENT=clientlib
              -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
      DEPENDS cw_client
      COMMENT "Install only the cw_client library (component: clientlib)"
    )
else()
    add_custom_target(install_client
    	COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --component clientlib
    	DEPENDS cw_client
    	COMMENT "Install only the cw_client library (component: clientlib)"
    )
endif()
#install(DIRECTORY src/ DESTINATION include/clockwork FILES_MATCHING PATTERN "*.h")
