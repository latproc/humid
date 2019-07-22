cmake_minimum_required(VERSION 3.0)
project("humid")

if (EXISTS "${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
	message("Including local CMake settings")
	include("${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()

if (MINGW_BUILD)
    include("lib/clockwork_client.git/cmake_modules/mingw_defines.cmake")
    get_property(CW_MINGW_DEFINITIONS GLOBAL PROPERTY CW_MINGW_DEFINITIONS)
    add_definitions(${CW_MINGW_DEFINITIONS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unknown-warning-option -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-int-in-bool-context -Wno-implicit-fallthrough -Wno-misleading-indentation")
set(NANOGUI_BUILD_PYTHON NO)

add_subdirectory(ext)
add_subdirectory(lib)
add_subdirectory(app)
