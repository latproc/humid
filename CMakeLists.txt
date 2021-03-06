cmake_minimum_required(VERSION 3.5)

project("Humid")

if (EXISTS "${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
	message("Including local CMake settings")
	include("${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")


set(NANOGUI_BUILD_SHARED ON)
if(APPLE AND NANOGUI_BUILD_SHARED)
	set(CMAKE_MACOSX_RPATH ON)
endif()

if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
	INCLUDE_DIRECTORIES(${NANO_GUI_BUILD_DIR}/include)
	INCLUDE_DIRECTORIES("/usr/local/include")
endif()

include(CheckCXXCompilerFlag)

set(CMAKE_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-parameter")

set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
add_subdirectory(lib/nanogui)
include_directories(lib/nanogui/include)
include_directories(${NANOGUI_EXTRA_INCS})
add_definitions(${NANOGUI_EXTRA_DEFS})

include_directories("/usr/local/include")
include_directories("/opt/local/include")

#LINK_DIRECTORIES("/usr/local/lib")
#LINK_DIRECTORIES("/opt/local/lib")

find_package(ZeroMQ REQUIRED)
find_package(CURL REQUIRED)

#set(BOOST_ROOT "/usr/local/")
FIND_PACKAGE(Boost COMPONENTS system thread program_options filesystem chrono date_time REQUIRED )

set (CLOCKWORK_DIR /opt/latproc/iod/src)
set(NANOGUI_EXTRA_INCS "CLOCKWORK_DIR")

set(HEADER_FILES
  ${CLOCKWORK_DIR}/MessagingInterface.h
  ${CLOCKWORK_DIR}/SocketMonitor.h
  ${CLOCKWORK_DIR}/ConnectionManager.h
  ${CLOCKWORK_DIR}/MessageLog.h
  ${CLOCKWORK_DIR}/MessageEncoding.h
  ${CLOCKWORK_DIR}/MessageHeader.h
  ${CLOCKWORK_DIR}/regular_expressions.h
  ${CLOCKWORK_DIR}/Logger.h
  ${CLOCKWORK_DIR}/anet.h
  ${CLOCKWORK_DIR}/cJSON.h
  ${CLOCKWORK_DIR}/DebugExtra.h
  ${CLOCKWORK_DIR}/value.h
  ${CLOCKWORK_DIR}/symboltable.h
  ${CLOCKWORK_DIR}/rate.h
  ${CLOCKWORK_DIR}/watchdog.h
  ${CLOCKWORK_DIR}/Message.h
  )

add_library (Clockwork
${CLOCKWORK_DIR}/MessagingInterface.cpp
${CLOCKWORK_DIR}/MessageLog.cpp
${CLOCKWORK_DIR}/MessageEncoding.cpp
${CLOCKWORK_DIR}/MessageHeader.cpp
${CLOCKWORK_DIR}/regular_expressions.cpp
${CLOCKWORK_DIR}/CommandManager.cpp
${CLOCKWORK_DIR}/SocketMonitor.cpp
${CLOCKWORK_DIR}/ConnectionManager.cpp
${CLOCKWORK_DIR}/Logger.cpp
${CLOCKWORK_DIR}/anet.c
${CLOCKWORK_DIR}/cJSON.c
${CLOCKWORK_DIR}/DebugExtra.cpp
${CLOCKWORK_DIR}/value.cpp
${CLOCKWORK_DIR}/symboltable.cpp
${CLOCKWORK_DIR}/rate.cpp
${CLOCKWORK_DIR}/Message.cpp
${CLOCKWORK_DIR}/watchdog.cpp
${HEADER_FILES}
)
target_link_libraries(Clockwork ${Boost_LIBRARIES} ${ZeroMQ_LIBRARY})
target_include_directories(Clockwork PUBLIC ${ZeroMQ_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})

include_directories("${CLOCKWORK_DIR}")

#string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(HEADERS src/draghandle.h	src/manuallayout.h	src/propertymonitor.h	src/skeleton.h
		  src/structure.h src/parameter.h src/hmilang.h src/lineplot.h src/circularbuffer.h
		  src/propertylist.h src/gltexture.h src/uiitem.h
		  src/selectable.h src/palette.h src/selectablewidget.h
		  src/selectablebutton.h src/userwindow.h src/sampletrigger.h
		  src/panelscreen.h src/namedobject.h src/editorproject.h
		  src/viewlistcontroller.h src/editorsettings.h
		  src/editorobject.h src/linkableobject.h src/editorgui.h
		  src/factorybuttons.h src/editor.h src/connectable.h
		  src/editorwidget.h src/editorbutton.h src/editortextbox.h src/editorlabel.h
		  src/editorimageview.h src/editorlineplot.h src/editorprogressbar.h
		  src/structureswindow.h src/themewindow.h src/propertywindow.h
		  src/helper.h src/curl_helper.h src/screenswindow.h
		  src/linkableproperty.h src/anchor.h src/objectwindow.h src/patternswindow.h
		  src/resourcemanager.h src/viewswindow.h src/listpanel.h
		  src/toolbar.h src/propertyformhelper.h src/propertyformwindow.h
		  src/startupwindow.h src/widgetfactory.h src/dialogwindow.h
		  src/editorframe.h
  )

#add_executable(okdialog src/simple_client.cpp  src/manuallayout.cpp src/skeleton.cpp src/panelscreen.cpp ${HEADERS})
#target_link_libraries(okdialog Clockwork ${ZeroMQ_LIBRARY} nanogui ${Boost_LIBRARIES})

# based on yacc instructions from Andy Cedilnik 2002.

set (FLEX_EXECUTABLE lex)
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
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.ypp
	   OUTPUT ${hmi_lex_output})

	# Create custom command for bison/yacc (note the DEPENDS)
	 ADD_CUSTOM_COMMAND(
	   COMMAND ${BISON_EXECUTABLE}
	   ARGS -y
			-o ${CMAKE_BINARY_DIR}/hmilang.tab.cpp
			-g -v -d ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/hmilang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/structure.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parameter.h
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
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.ypp
	   OUTPUT ${settings_lex_output})

	# Create custom command for bison/yacc (note the DEPENDS)
	 ADD_CUSTOM_COMMAND(
	   COMMAND ${BISON_EXECUTABLE}
	   ARGS -y
			-o ${CMAKE_BINARY_DIR}/settingslang.tab.cpp
			-g -v -d ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.lpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/settingslang.ypp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/structure.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parameter.h
	   OUTPUT ${settings_yacc_output})

	# Add generated source to the list of sources
	SET(SETTINGCHECK_PARSER src/settingslang.lpp src/namedobject.cpp src/settingslang.ypp ${settings_yacc_output} ${settings_lex_output})

	# Since the parser source does not exist yet when cmake is run, mark
	# it as generated
	SET_SOURCE_FILES_PROPERTIES(${settings_lex_output} GENERATED)
	SET_SOURCE_FILES_PROPERTIES(${settings_yacc_output} GENERATED)

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
		  src/curl_helper.cpp src/screenswindow.cpp
		  src/linkableproperty.cpp src/anchor.cpp src/objectwindow.cpp src/patternswindow.cpp
		  src/resourcemanager.cpp src/viewswindow.cpp src/listpanel.cpp
		  src/toolbar.cpp src/propertyformhelper.cpp src/propertyformwindow.cpp
		  src/userwindow.cpp src/startupwindow.cpp
		  src/widgetfactory.cpp src/dialogwindow.cpp
		  src/editorframe.cpp
		  ${HEADERS})

  target_link_libraries(humid nanogui ${NANOGUI_EXTRA_LIBS} ${CURL_LIBRARIES} Clockwork  ${ZeroMQ_LIBRARY}  ${Boost_LIBRARIES})

  add_executable(settingscheck ${SETTINGCHECK_PARSER} src/settingscheck.cpp src/parameter.cpp src/structure.cpp)
  target_link_libraries(settingscheck Clockwork  ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})

  SET(HMICHECK_PARSER src/hmilang.lpp src/hmilang.ypp ${hmi_yacc_output} ${hmi_lex_output})
  add_executable(hmifile_check ${HMICHECK_PARSER} src/hmifile_check.cpp src/parameter.cpp src/structure.cpp
  		src/namedobject.cpp src/anchor.cpp
	)
  target_link_libraries(hmifile_check nanogui ${NANOGUI_EXTRA_LIBS} Clockwork ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})


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
