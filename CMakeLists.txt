cmake_minimum_required(VERSION 3.0)
project("humid")

if (EXISTS "${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
	message("Including local CMake settings")
	include("${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()


if (MINGW_BUILD)
    link_directories(
        /usr/x86_64-w64-mingw32/lib
        /usr/x86_64-w64-mingw32/lib64
    )
    include_directories(
        /usr/x86_64-w64-mingw32/include
    )
    add_definitions(-DMINGW_BUILD=1)
    add_definitions(-include winsock2.h)
    add_definitions(-include windows.h)
    add_definitions(-include stdint.h)
    add_definitions(-D_USE_MATH_DEFINES)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

else()

endif()


#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unknown-warning-option -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-int-in-bool-context -Wno-implicit-fallthrough -Wno-misleading-indentation")
set(NANOGUI_BUILD_PYTHON NO)

add_subdirectory(ext)

add_subdirectory(lib)
add_subdirectory(app)
