cmake_minimum_required(VERSION 3.0)
project("my_project")

if (EXISTS "${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
	message("Including local CMake settings")
	include("${PROJECT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()

# cmake .. -DMINGW_BUILD=YES -DCMAKE_TOOLCHAIN_FILE=../lib/clockwork/cmake_modules/mingw_toolchain.cmake -DNANOGUI_BUILD_PYTHON=NO

if (MINGW_BUILD) #copy_paste from libdruid.git/
    # cmake ../simple.git/ -DMINGW_BUILD=YES -DCMAKE_TOOLCHAIN_FILE=../simple.git/lib/libdruid.git/cmake_modules/mingw_toolchain.cmake
    # add_definitions(-DCMAKE_TOOLCHAIN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/lib/libdruid.git/cmake_modules/mingw_toolchain.cmake")
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

    # add_definitions(-include mingw.thread.h)

    # defs for gecode
    # add_definitions(-D__x86_64__)
    # add_definitions(-D__USE_ISOC99)

    # add_definitions(-DWIN32_LEAN_AND_MEAN)

else()

endif()


#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unknown-warning-option -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-int-in-bool-context -Wno-implicit-fallthrough -Wno-misleading-indentation")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-int-in-bool-context -Wno-implicit-fallthrough")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#add_definitions(-std=c++11)
set(NANOGUI_BUILD_PYTHON NO)

add_subdirectory(ext)

if (MINGW_BUILD)
    # add_definitions(-std=c++11)
    # add_definitions(-include mingw.condition_variable.h)
    # add_definitions(-include mingw.mutex.h)
    # add_definitions(-include mingw.shared_mutex.h)
    # add_definitions(-include mingw.future.h)
endif()
add_subdirectory(lib)
add_subdirectory(app)
