# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
# SET(CMAKE_C_COMPILER i686-w64-mingw32-g++)
# SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
# SET(CMAKE_RC_COMPILER i686-w64-mingw32-g++)

SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-win32)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-win32)
SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-g++-win32)

# SET(CMAKE_SHARED_LINKER_FLAGS "-zmuldefs")
# SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,multidefs")
# SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,-b,svr4,-z,multidefs")
# SET(CMAKE_SHARED_LINKER_FLAGS "--allow-multiple-definition")


# SET(CMAKE_SHARED_LINKER_FLAGS "-Xlinker -z muldefs")
# SET(CMAKE_EXE_LINKER_FLAGS )
# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32/)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
