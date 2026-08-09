cmake_minimum_required(VERSION 3.5)

project("Humid" C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/stage")

if (EXISTS "${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
	message("Including local CMake settings")
	include("${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")


set(NANOGUI_BUILD_SHARED ON)
set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL "Build NanoGUI example application?" FORCE)
if(APPLE AND NANOGUI_BUILD_SHARED)
	set(CMAKE_MACOSX_RPATH ON)
endif()

# ---------------------------------------------------------------------------
# Platform auto-detection (build time)
# OpenGL ES is preferred on ARM / Raspberry Pi; desktop OpenGL elsewhere.
# Override any time with -DNANOGUI_USE_GLES=ON|OFF.
# Wayland vs X11 is selected by GLFW at *run* time when both are built.
# ---------------------------------------------------------------------------
set(_HUMID_IS_ARM FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64|armv[0-9]|arm)")
  set(_HUMID_IS_ARM TRUE)
endif()

set(_HUMID_IS_RPI FALSE)
if(CMAKE_TOOLCHAIN_FILE MATCHES "RaspberryPi")
  set(_HUMID_IS_RPI TRUE)
elseif(UNIX AND NOT APPLE)
  if(EXISTS "/proc/device-tree/model")
    file(READ "/proc/device-tree/model" _HUMID_DT_MODEL LIMIT 256)
    if(_HUMID_DT_MODEL MATCHES "[Rr]aspberry")
      set(_HUMID_IS_RPI TRUE)
    endif()
  endif()
  if(NOT _HUMID_IS_RPI)
    if(EXISTS "/etc/rpi-issue" OR EXISTS "/usr/bin/raspi-config")
      set(_HUMID_IS_RPI TRUE)
    endif()
  endif()
endif()

# Default GLES on Linux ARM / Raspberry Pi only. Do not force GLES on
# Apple Silicon (aarch64 Darwin) or other desktop hosts — use desktop GL.
set(_HUMID_DEFAULT_GLES OFF)
if(NOT APPLE AND (_HUMID_IS_ARM OR _HUMID_IS_RPI))
  set(_HUMID_DEFAULT_GLES ON)
endif()

option(NANOGUI_USE_GLES
  "Use OpenGL ES 3.0 instead of desktop OpenGL (auto ON for ARM/RPi)"
  ${_HUMID_DEFAULT_GLES})

# On GLES targets (typical RPi), build both GLFW backends so the running
# compositor can pick Wayland or X11. Desktop x86 panels keep GLFW defaults.
# Use plain DEFINED (not CACHE{}) for CMake 3.5 panel compatibility.
if(UNIX AND NOT APPLE AND NANOGUI_USE_GLES)
  if(NOT DEFINED GLFW_BUILD_WAYLAND)
    set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build support for Wayland window system")
  endif()
  if(NOT DEFINED GLFW_BUILD_X11)
    set(GLFW_BUILD_X11 ON CACHE BOOL "Build support for X11 window system")
  endif()
endif()

message(STATUS "Humid platform detect: processor=${CMAKE_SYSTEM_PROCESSOR} arm=${_HUMID_IS_ARM} rpi=${_HUMID_IS_RPI} NANOGUI_USE_GLES=${NANOGUI_USE_GLES}")

set(CMAKE_CXX_STANDARD 14)
if (APPLE)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 ")
	INCLUDE_DIRECTORIES(${NANO_GUI_BUILD_DIR}/include)
	INCLUDE_DIRECTORIES("/usr/local/include")
endif()

include(CheckCXXCompilerFlag)

# Default to Release when no build type is given. Debug layout of large HTML
# manuals is much slower; override with -DCMAKE_BUILD_TYPE=Debug if needed.
# Output always lives under the normal out-of-tree dir (e.g. build/), not
# build/Release/ — multi-config generators are unaffected.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
if(CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
  message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

# Apple Clang 21 can ICE (segfault) on some NanoGUI TUs at -O3 (e.g. glutil.cpp).
# Use -O2 for Release on Apple Clang; still far faster than unoptimized Debug.
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if(NOT DEFINED HUMID_APPLE_RELEASE_FLAGS_SET)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags for Release" FORCE)
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags for Release" FORCE)
    set(HUMID_APPLE_RELEASE_FLAGS_SET TRUE CACHE INTERNAL "")
    message(STATUS "Apple Clang: Release uses -O2 (avoids -O3 frontend crash on nanogui)")
  endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
if (USE_ASAN)
    message(STATUS "ASAN build")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined")
endif()
if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-unused-parameter")

set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
add_subdirectory(lib/nanogui)
# Apple Clang 21 ICE/segfaults optimizing some NanoGUI TUs (e.g. glutil.cpp at -O2).
# Force -O0 on the nanogui object target only; humid_litehtml / humid stay Release-opt.
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND TARGET nanogui-obj)
  target_compile_options(nanogui-obj PRIVATE -O0)
  message(STATUS "Apple Clang: nanogui-obj forced to -O0 (compiler ICE workaround)")
endif()
if (NANOGUI_USE_GLES)
  # Ensure NanoGUI object sources and humid share the same GLES compile path.
  if (TARGET nanogui-obj)
    target_compile_definitions(nanogui-obj PRIVATE NANOGUI_GLES)
  endif()
  add_definitions(-DHUMID_USE_GLES=1)
else()
  add_definitions(-DHUMID_USE_GLES=0)
endif()
set_target_properties(nanogui PROPERTIES
		INSTALL_RPATH "@loader_path/../lib"
		BUILD_WITH_INSTALL_RPATH TRUE)
include_directories(lib/nanogui/include)
include_directories(${NANOGUI_EXTRA_INCS})
add_definitions(${NANOGUI_EXTRA_DEFS})
#install(TARGETS nanogui LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

include_directories("/usr/local/include")
include_directories("/opt/local/include")
#LINK_DIRECTORIES("/usr/local/lib")
#LINK_DIRECTORIES("/opt/local/lib")

find_package(ZeroMQ REQUIRED)
find_package(ClockworkClient REQUIRED)
find_package(CURL REQUIRED)

# Optional embedded HTMLVIEW (litehtml + Cairo/Pango). Open-source only.
include(HumidLitehtml)

message(STATUS "Found clockwork: ${ClockworkClient_LIBRARIES}")
message(STATUS "Clockwork includes: ${ClockworkClient_INCLUDE_DIR}")
# Guard against stale CMake cache / LocalCMakeLists pointing at /opt/latproc
# while sources expect the submodule client API (e.g. addSetupResponder).
if (ClockworkClient_LIBRARIES AND NOT ClockworkClient_LIBRARIES MATCHES "clockwork")
  message(WARNING
    "Clockwork client is not from the humid submodule:\n"
    "  ${ClockworkClient_LIBRARIES}\n"
    "After 'cd clockwork && make client-install', reconfigure so humid "
    "uses clockwork/stage/lib/libcw_client.a")
endif()

#set(BOOST_ROOT "/usr/local/")
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")
FIND_PACKAGE(Boost COMPONENTS thread program_options filesystem chrono date_time REQUIRED )

# Ubuntu 18.04 production panels may have Boost 1.74 headers together with a
# header-only date_time compatibility library. Clockwork still uses the legacy
# greg_month string symbols, which remain available in the co-installed 1.65
# library. Append it after the normal Boost libraries when it is present.
find_library(Boost_DATE_TIME_LEGACY_LIBRARY
  NAMES boost_date_time.so.1.65.1 libboost_date_time.so.1.65.1
  PATHS /usr/lib/x86_64-linux-gnu
  NO_DEFAULT_PATH)
if (Boost_DATE_TIME_LEGACY_LIBRARY)
  list(APPEND Boost_LIBRARIES ${Boost_DATE_TIME_LEGACY_LIBRARY})
endif()

set (CLOCKWORK_DIR "${PROJECT_SOURCE_DIR}/clockwork/src")
set(NANOGUI_EXTRA_INCS "CLOCKWORK_DIR")
#set(Clockwork_INCLUDE_DIR /opt/latproc/stage/include/clockwork)
#set(ClockworkClient_LIBRARIES /opt/latproc/stage/lib/libcw_client.a)

if (RUN_TESTS)
enable_testing()
add_subdirectory(tests)
endif()

set(Clockwork_INCLUDE_DIR "${CLOCKWORK_DIR}")

include_directories("${ClockworkClient_INCLUDE_DIR}")

#string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(HEADERS src/draghandle.h	src/manuallayout.h	src/propertymonitor.h	src/skeleton.h
		  src/structure.h  src/hmilang.h src/lineplot.h src/circularbuffer.h
		  src/propertylist.h src/gltexture.h src/uiitem.h
		  src/selectable.h src/palette.h src/selectablewidget.h
		  src/selectablebutton.h src/userwindow.h src/sampletrigger.h
		  src/panelscreen.h src/namedobject.h src/editorproject.h
		  src/viewlistcontroller.h src/editorsettings.h
		  src/editorobject.h src/linkableobject.h src/editorgui.h
		  src/factorybuttons.h src/editor.h src/connectable.h
		  src/editorwidget.h src/editorbutton.h src/editortextbox.h src/editorlabel.h
		  src/editorimageview.h src/editorlineplot.h src/editorprogressbar.h src/structureswindow.h src/themewindow.h src/propertywindow.h
		  src/helper.h src/curl_helper.h src/screenswindow.h
		  src/linkableproperty.h src/anchor.h src/objectwindow.h src/patternswindow.h
		  src/resourcemanager.h src/viewswindow.h src/listpanel.h
		  src/toolbar.h src/propertyformhelper.h src/propertyformwindow.h
		  src/startupwindow.h src/widgetfactory.h src/dialogwindow.h
		  src/editorframe.h src/keypad.h src/valuehelper.h
		  src/linkmanager.h src/thememanager.h src/colourhelper.h
		  src/screencapture.h
		  src/editortable.h
  )
if (HUMID_HTMLVIEW_FOUND)
  list(APPEND HEADERS src/editorhtmlview.h src/htmlview_container.h)
endif()

#add_executable(okdialog src/simple_client.cpp  src/manuallayout.cpp src/skeleton.cpp src/panelscreen.cpp ${HEADERS})
#target_link_libraries(okdialog ${ClockworkClient_LIBRARIES} ${ZeroMQ_LIBRARY} nanogui ${Boost_LIBRARIES})

# based on yacc instructions from Andy Cedilnik 2002.

set (FLEX_EXECUTABLE flex)
if (BISON_EXECUTABLE)
else()
	set (BISON_EXECUTABLE bison)
endif ()

	set(hmi_lex_output ${CMAKE_BINARY_DIR}/hmilang.yy.cpp)
	set(hmi_yacc_output ${CMAKE_BINARY_DIR}/hmilang.tab.cpp)
	# Create custom command for flex/lex (note the outputs)
	 ADD_CUSTOM_COMMAND(
	   COMMAND ${FLEX_EXECUTABLE}
	   ARGS -o${CMAKE_BINARY_DIR}/hmilang.yy.cpp
	   ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.ypp
	   OUTPUT ${hmi_lex_output})

	# Create custom command for bison/yacc (note the DEPENDS)
	 ADD_CUSTOM_COMMAND(
	   COMMAND ${BISON_EXECUTABLE}
	   ARGS
			-o ${CMAKE_BINARY_DIR}/hmilang.tab.cpp
			-v -d ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/structure.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/clockwork/src/Parameter.h
	   OUTPUT ${hmi_yacc_output})

	# Add generated source to the list of sources
	 SET(HMI_SRCS ${HMI_SRCS} src/hmilang.lpp src/hmilang.ypp ${hmi_yacc_output} ${hmi_lex_output})

	# Since the parser source does not exist when cmake is run, mark
	# it as generated
	SET_SOURCE_FILES_PROPERTIES(${hmi_lex_output} GENERATED)
	SET_SOURCE_FILES_PROPERTIES(${hmi_yacc_output} GENERATED)

	# Include binary directory to include lexer.c in parser.c
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
	INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src)

	set(settings_lex_output ${CMAKE_BINARY_DIR}/settingslang.yy.cpp)
	set(settings_yacc_output ${CMAKE_BINARY_DIR}/settingslang.tab.cpp)
	# Create custom command for flex/lex (note the outputs)
	 ADD_CUSTOM_COMMAND(
	   COMMAND ${FLEX_EXECUTABLE}
	   ARGS -o${CMAKE_BINARY_DIR}/settingslang.yy.cpp
	   ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.ypp
	   OUTPUT ${settings_lex_output})

	# Create custom command for bison/yacc (note the DEPENDS)
	 ADD_CUSTOM_COMMAND(
	   COMMAND ${BISON_EXECUTABLE}
	   ARGS
			-o ${CMAKE_BINARY_DIR}/settingslang.tab.cpp
			-v -d ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/structure.h
		DEPENDS   ${Clockwork_INCLUDE_DIR}/Parameter.h
	   OUTPUT ${settings_yacc_output})

	# Add generated source to the list of sources
	SET(SETTINGCHECK_PARSER src/settingslang.lpp src/namedobject.cpp src/settingslang.ypp ${settings_yacc_output} ${settings_lex_output})

	# Since the parser source does not exist yet when cmake is run, mark
	# it as generated
	SET_SOURCE_FILES_PROPERTIES(${settings_lex_output} GENERATED)
	SET_SOURCE_FILES_PROPERTIES(${settings_yacc_output} GENERATED)

	#	ADD_CUSTOM_COMMAND(
	#			COMMAND protoc
	#			ARGS
	#				--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/src
	#				project.proto
	#				--cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/src
	#			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/project.proto
	#			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/project.pb.cc
	#			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/project.pb.h
	#	)

    #find_package(Protobuf)
if (Protobuf_INCLUDE_DIRS)
	include_directories(${Protobuf_INCLUDE_DIRS})
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS src/project.proto)
	#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS EXPORT_MACRO DLL_EXPORT src/project.proto)
	#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS DESCRIPTORS PROTO_DESCS src/project.proto)
	add_executable(check_project src/check_project.cpp ${PROTO_SRCS} ${PROTO_HDRS})
	target_link_libraries(check_project ${Protobuf_LIBRARIES})

	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/src/project.pb.h GENERATED)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/src/project.pb.cc GENERATED)
endif()

  set(HUMID_EXTRA_SOURCES "")
  if (HUMID_HTMLVIEW_FOUND)
    list(APPEND HUMID_EXTRA_SOURCES ${HUMID_HTMLVIEW_SOURCES})
  endif()

  add_executable(humid ${HMI_SRCS} src/humid.cpp  src/manuallayout.cpp src/skeleton.cpp
		  src/propertymonitor.cpp src/draghandle.cpp src/structure.cpp src/parameter.cpp
		  src/lineplot.cpp src/circularbuffer.cpp src/propertylist.cpp
		  src/gltexture.cpp src/uiitem.cpp src/selectable.cpp
		  src/palette.cpp src/selectablewidget.cpp
		  src/selectablebutton.cpp
		  src/panelscreen.cpp
		  src/namedobject.cpp
		  src/sampletrigger.cpp ${SETTINGCHECK_PARSER}
		  src/editorproject.cpp src/viewlistcontroller.cpp
		  src/editorsettings.cpp src/editorobject.cpp
		  src/linkableobject.cpp src/editorgui.cpp src/factorybuttons.cpp
		  src/editor.cpp src/connectable.cpp src/editorwidget.cpp
		  src/editorbutton.cpp src/editortextbox.cpp src/editorlabel.cpp
		  src/editorimageview.cpp src/editorlineplot.cpp
		  src/editorprogressbar.cpp src/structureswindow.cpp
		  src/themewindow.cpp src/propertywindow.cpp src/helper.cpp
		  src/platformdetect.cpp
		  src/curl_helper.cpp src/screenswindow.cpp
		  src/linkableproperty.cpp src/anchor.cpp src/objectwindow.cpp src/patternswindow.cpp
		  src/resourcemanager.cpp src/viewswindow.cpp src/listpanel.cpp
		  src/toolbar.cpp src/propertyformhelper.cpp src/propertyformwindow.cpp
		  src/userwindow.cpp src/startupwindow.cpp
		  src/widgetfactory.cpp src/dialogwindow.cpp
		  src/editorframe.cpp src/keypad.cpp src/valuehelper.cpp
		  src/linkmanager.cpp src/thememanager.cpp src/colourhelper.cpp
		  src/screencapture.cpp
		  src/editortable.cpp
		  src/anchor_widget.cpp
		  ${HUMID_EXTRA_SOURCES}
		  ${HEADERS})

  target_link_libraries(humid nanogui ${NANOGUI_EXTRA_LIBS} ${CURL_LIBRARIES} ${ClockworkClient_LIBRARIES} ${ZeroMQ_LIBRARY}  ${Boost_LIBRARIES})
  if (HUMID_HTMLVIEW_FOUND)
    target_include_directories(humid PRIVATE ${HUMID_HTMLVIEW_INCS})
    target_compile_definitions(humid PRIVATE ${HUMID_HTMLVIEW_DEFS})
    # HTMLVIEW TUs need C++17 for litehtml headers
    set_source_files_properties(${HUMID_HTMLVIEW_SOURCES} PROPERTIES COMPILE_FLAGS "-std=c++17")
    target_link_libraries(humid ${HUMID_HTMLVIEW_LIBS})
  endif()

  install(TARGETS humid RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/stage/bin)

  add_executable(settingscheck ${SETTINGCHECK_PARSER} src/settingscheck.cpp src/parameter.cpp src/structure.cpp src/linkmanager.cpp)
  target_link_libraries(settingscheck ${ClockworkClient_LIBRARIES} ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})

  # Headless .humid parser/checker — no NanoGUI/GLFW/display (safe on panels).
  SET(HMICHECK_PARSER src/hmilang.lpp src/hmilang.ypp ${hmi_yacc_output} ${hmi_lex_output})
  add_executable(hmifile_check ${HMICHECK_PARSER} src/hmifile_check.cpp src/parameter.cpp src/structure.cpp
  		src/namedobject.cpp src/anchor.cpp src/valuehelper.cpp src/linkmanager.cpp
	)
  target_link_libraries(hmifile_check ${ClockworkClient_LIBRARIES} ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})

  install(TARGETS hmifile_check RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/stage/bin)


  # Copy icons for example application
  file(COPY resources/images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  #file(COPY CLOCKWORK_tags.csv DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  #file(COPY 2GRAB_tags.csv DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

get_directory_property(NANOGUI_HAS_PARENT PARENT_DIRECTORY)
if(NANOGUI_HAS_PARENT)
  # This project is included from somewhere else. Export NANOGUI_EXTRA_LIBS variable
  set(NANOGUI_EXTRA_LIBS ${NANOGUI_EXTRA_LIBS} PARENT_SCOPE)
  set(NANOGUI_EXTRA_DEFS ${NANOGUI_EXTRA_DEFS} PARENT_SCOPE)
  set(NANOGUI_EXTRA_INCS ${NANOGUI_EXTRA_INCS} PARENT_SCOPE)
else()
  # Create documentation for python plugin (optional target for developers)
  add_custom_target(mkdoc COMMAND
		  python3 ${NANO_GUI_BUILD_DIR}/ext/pybind11/tools/mkdoc.py
		  #		  -I${NANO_GUI_BUILD_DIR}/include
		  -I${NANO_GUI_BUILD_DIR}/ext/nanovg/src
		  -I${NANO_GUI_BUILD_DIR}/ext/eigen -I${NANO_GUI_BUILD_DIR}/ext/glfw/include
	${NANO_GUI_BUILD_DIR}/include/nanogui/*.h > ${NANO_GUI_BUILD_DIR}/python/py_doc.h)
endif()
