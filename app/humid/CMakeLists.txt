cmake_minimum_required(VERSION 3.5)

project("Humid")

set (RUN_TESTS OFF)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 ")

set(CMAKE_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-lto -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-parameter")

set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)


# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

get_property(ZMQ-Dep_INCLUDE_DIRS GLOBAL PROPERTY ZMQ-Dep_INCLUDE_DIRS)
get_property(Boost-Dep_INCLUDE_DIRS GLOBAL PROPERTY Boost-Dep_INCLUDE_DIRS)
get_property(Curl-Dep_INCLUDE_DIRS GLOBAL PROPERTY Curl-Dep_INCLUDE_DIRS)
get_property(Clockwork_INCLUDE_DIRS GLOBAL PROPERTY Clockwork_INCLUDE_DIRS)
get_property(nanogui_INCLUDE_DIRS GLOBAL PROPERTY nanogui_INCLUDE_DIRS)
# message("NANOGUI EXTRA LIBS: ${NANOGUI_EXTRA_LIBS}")
set(INCLUDE
    ${ZMQ_HPP-Dep_INCLUDE_DIRS}
    ${_ZMQ-Dep_INCLUDE_DIRS}
    ${Boost-Dep_INCLUDE_DIRS}
    ${Curl-Dep_INCLUDE_DIRS}
    ${Clockwork_INCLUDE_DIRS}
    ${nanogui_INCLUDE_DIRS}

    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if (MINGW_BUILD)
set(LIBRARY
    # ZMQ-Dep
    # Boost-Dep
    # Curl-Dep
    Clockwork
    nanogui
    # DruidOpenGL-Dep
    opengl32
)
else()
set(LIBRARY
    # ZMQ-Dep
    # Boost-Dep
    # Curl-Dep
    Clockwork
    nanogui
    # DruidOpenGL-Dep
    # opengl32
)
endif()

include_directories(${INCLUDE})
# message("HUMID INCLUDE DIRS: " ${INCLUDE})
# message("HUMID LIBRARY: " ${LIBRARY})

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
		  src/resourcemanager.h src/shrinkable.h
)

if (RUN_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#add_executable(okdialog src/simple_client.cpp  src/manuallayout.cpp src/skeleton.cpp src/panelscreen.cpp ${HEADERS})
#target_link_libraries(okdialog Clockwork ${ZeroMQ_LIBRARY} nanogui ${Boost_LIBRARIES})

# based on yacc instructions from Andy Cedilnik 2002.

set(FLEX_EXECUTABLE lex)
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
    OUTPUT ${hmi_lex_output}
)

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
    OUTPUT ${hmi_yacc_output}
)

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
    OUTPUT ${settings_lex_output}
)

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
   OUTPUT ${settings_yacc_output}
)

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
	  src/resourcemanager.cpp src/shrinkable.cpp
	  ${HEADERS}
)

if (MINGW_BUILD)
add_definitions(
    "-DGLAD_GLAPI_EXPORT"
    # "MINGW_BUILD=1"
    "-DNANOGUI_GLAD"
    "-DNANOGUI_SHARED"
    "-DNVG_SHARED"
    # "_USE_MATH_DEFINES"
)
else()
add_definitions(
    # "-DGLAD_GLAPI_EXPORT"
    # "MINGW_BUILD=1"
    # "-DNANOGUI_GLAD"
    "-DNANOGUI_SHARED"
    "-DNVG_SHARED"
    # "_USE_MATH_DEFINES"
)
endif()

message("HUMID LIBRARIES: ${LIBRARY}")
target_link_libraries(humid ${LIBRARY})
# target_link_libraries(humid nanogui ${NANOGUI_EXTRA_LIBS} ${CURL_LIBRARIES} Clockwork ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})

add_executable(settingscheck ${SETTINGCHECK_PARSER} src/settingscheck.cpp src/parameter.cpp src/structure.cpp)
# target_link_libraries(settingscheck Clockwork  ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})
# target_link_libraries(settingscheck Clockwork ${LIBRARY})
target_link_libraries(settingscheck Clockwork ZMQ-Dep Boost-Dep)
#
# # ${ZMQ_HPP-Dep}
# # ${_ZMQ-Dep}
#
# ${Curl-Dep}
# Clockwork
# nanogui



if (NO)
    SET(HMICHECK_PARSER src/hmilang.lpp src/hmilang.ypp ${hmi_yacc_output} ${hmi_lex_output})

    add_executable(hmifile_check ${HMICHECK_PARSER} src/hmifile_check.cpp src/parameter.cpp src/structure.cpp
        src/namedobject.cpp src/anchor.cpp
    )

    target_link_libraries(hmifile_check nanogui ${NANOGUI_EXTRA_LIBS} Clockwork ${ZeroMQ_LIBRARY} ${Boost_LIBRARIES})

endif()

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
