# Optional litehtml + Cairo HTMLVIEW support for Humid.
# Compatible with CMake 3.5 (does not use add_subdirectory of litehtml's 3.11 CMake).

option(HUMID_WITH_HTMLVIEW
  "Build embedded HTMLVIEW (litehtml + Cairo/Pango)"
  ON)

set(HUMID_HTMLVIEW_FOUND FALSE)
set(HUMID_HTMLVIEW_LIBS "")
set(HUMID_HTMLVIEW_INCS "")
set(HUMID_HTMLVIEW_SOURCES "")
set(HUMID_HTMLVIEW_DEFS "")

if(NOT HUMID_WITH_HTMLVIEW)
  message(STATUS "HTMLVIEW: disabled (HUMID_WITH_HTMLVIEW=OFF)")
  return()
endif()

find_package(PkgConfig QUIET)
if(NOT PkgConfig_FOUND AND NOT PKG_CONFIG_FOUND)
  find_program(PKG_CONFIG_EXECUTABLE pkg-config)
endif()

set(_HUMID_HAVE_CAIRO FALSE)
set(_HUMID_HTMLVIEW_MISSING_PKGS "")
if(PKG_CONFIG_EXECUTABLE OR PkgConfig_FOUND OR PKG_CONFIG_FOUND)
  find_package(PkgConfig QUIET)
  if(COMMAND pkg_check_modules)
    # Probe each module so the warning can name what is missing.
    # CMake 3.5: expand FOUND vars via an intermediate name.
    foreach(_mod cairo pangocairo fontconfig)
      string(TOUPPER "${_mod}" _mod_u)
      set(_pc_prefix "HUMID_PC_${_mod_u}")
      pkg_check_modules(${_pc_prefix} QUIET ${_mod})
      set(_pc_found_var "${_pc_prefix}_FOUND")
      if(NOT ${_pc_found_var})
        list(APPEND _HUMID_HTMLVIEW_MISSING_PKGS ${_mod})
      endif()
    endforeach()
    # Combined flags/libs for the link line (same modules).
    pkg_check_modules(HUMID_CAIRO QUIET cairo pangocairo fontconfig)
    if(HUMID_CAIRO_FOUND)
      set(_HUMID_HAVE_CAIRO TRUE)
    endif()
  else()
    list(APPEND _HUMID_HTMLVIEW_MISSING_PKGS "pkg-config (pkg_check_modules unavailable)")
  endif()
else()
  list(APPEND _HUMID_HTMLVIEW_MISSING_PKGS "pkg-config")
endif()

if(NOT _HUMID_HAVE_CAIRO)
  string(REPLACE ";" ", " _HUMID_HTMLVIEW_MISSING_STR "${_HUMID_HTMLVIEW_MISSING_PKGS}")
  if(NOT _HUMID_HTMLVIEW_MISSING_STR)
    set(_HUMID_HTMLVIEW_MISSING_STR "cairo, pangocairo, and/or fontconfig")
  endif()
  message(WARNING
    "HTMLVIEW: missing pkg-config modules: ${_HUMID_HTMLVIEW_MISSING_STR}\n"
    "  Building humid without HTMLVIEW (operators-manual viewer) this configure.\n"
    "  HUMID_WITH_HTMLVIEW stays ON so a later cmake run can enable it after:\n"
    "  Debian / Ubuntu / Raspberry Pi OS:\n"
    "    sudo apt-get install -y libcairo2-dev libpango1.0-dev libfontconfig1-dev pkg-config\n"
    "  macOS (Homebrew):\n"
    "    brew install cairo pango fontconfig pkg-config\n"
    "  Verify:\n"
    "    pkg-config --exists cairo pangocairo fontconfig && echo OK\n"
    "  Then re-run cmake (no need to delete CMakeCache.txt).\n"
    "  Or disable: cmake .. -DHUMID_WITH_HTMLVIEW=OFF")
  # Do not FORCE the option OFF. A previous missing-package probe used to
  # latch the cache, so later configures skipped HTMLVIEW even after the
  # development packages were installed.
  return()
endif()

set(LITEHTML_ROOT "${PROJECT_SOURCE_DIR}/lib/litehtml")
if(NOT EXISTS "${LITEHTML_ROOT}/include/litehtml.h")
  message(WARNING
    "HTMLVIEW: lib/litehtml missing at ${LITEHTML_ROOT}\n"
    "  Building humid without HTMLVIEW this configure.\n"
    "  Ensure the feature branch includes the vendored submodule/tree:\n"
    "    git submodule update --init --recursive\n"
    "  or restore lib/litehtml/include/litehtml.h\n"
    "  Or disable: cmake .. -DHUMID_WITH_HTMLVIEW=OFF")
  return()
endif()

# litehtml headers need C++17 <variant>. GCC 5 (Ubuntu 16.04) accepts -std=c++17
# but has no <variant>; do not enable HTMLVIEW on that compiler.
include(CheckCXXSourceCompiles)
set(_HUMID_SAVED_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++17")
check_cxx_source_compiles("
#include <variant>
int main() {
  std::variant<int, double> v = 1;
  return std::get<int>(v) - 1;
}
" HUMID_HAVE_CXX17_VARIANT)
set(CMAKE_REQUIRED_FLAGS "${_HUMID_SAVED_REQUIRED_FLAGS}")
if(NOT HUMID_HAVE_CXX17_VARIANT)
  message(WARNING
    "HTMLVIEW: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} "
    "(${CMAKE_CXX_COMPILER}) cannot compile C++17 <variant>.\n"
    "  litehtml needs GCC 7+, Clang 5+, or equivalent.\n"
    "  Ubuntu 16.04 default g++-5 cannot build the operators-manual viewer.\n"
    "  Building humid without HTMLVIEW this configure.\n"
    "  Install g++-7 or newer and re-run cmake, or pass "
    "-DCMAKE_CXX_COMPILER=g++-7")
  return()
endif()

file(GLOB HUMID_LITEHTML_CPP "${LITEHTML_ROOT}/src/*.cpp")
file(GLOB HUMID_GUMBO_C "${LITEHTML_ROOT}/src/gumbo/*.c")
set(HUMID_LITEHTML_CAIRO
  "${LITEHTML_ROOT}/containers/cairo/container_cairo.cpp"
  "${LITEHTML_ROOT}/containers/cairo/container_cairo_pango.cpp"
  "${LITEHTML_ROOT}/containers/cairo/cairo_borders.cpp"
  "${LITEHTML_ROOT}/containers/cairo/conic_gradient.cpp"
)

add_library(humid_litehtml STATIC
  ${HUMID_LITEHTML_CPP}
  ${HUMID_GUMBO_C}
  ${HUMID_LITEHTML_CAIRO}
)
target_include_directories(humid_litehtml PUBLIC
  "${LITEHTML_ROOT}/include"
  "${LITEHTML_ROOT}/containers/cairo"
  "${LITEHTML_ROOT}/src/gumbo/include"
  ${HUMID_CAIRO_INCLUDE_DIRS}
)
# litehtml sources #include "codepoint.h" etc. from include/litehtml
# gumbo sources #include "attribute.h" from src/gumbo/include/gumbo
target_include_directories(humid_litehtml PRIVATE
  "${LITEHTML_ROOT}/include/litehtml"
  "${LITEHTML_ROOT}/src"
  "${LITEHTML_ROOT}/src/gumbo/include/gumbo"
  "${LITEHTML_ROOT}/src/gumbo"
)
# litehtml sources need C++17; gumbo is C.
set_property(TARGET humid_litehtml PROPERTY CXX_STANDARD 17)
set_property(TARGET humid_litehtml PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_options(humid_litehtml PRIVATE ${HUMID_CAIRO_CFLAGS_OTHER})
# CMake 3.5: avoid target_link_directories; pass -L via link flags if needed.
if(HUMID_CAIRO_LIBRARY_DIRS)
  foreach(_d ${HUMID_CAIRO_LIBRARY_DIRS})
    target_link_libraries(humid_litehtml PUBLIC "-L${_d}")
  endforeach()
endif()
target_link_libraries(humid_litehtml PUBLIC ${HUMID_CAIRO_LIBRARIES})

set(HUMID_HTMLVIEW_FOUND TRUE)
set(HUMID_HTMLVIEW_LIBS humid_litehtml)
set(HUMID_HTMLVIEW_INCS
  "${LITEHTML_ROOT}/include"
  "${LITEHTML_ROOT}/containers/cairo"
  ${HUMID_CAIRO_INCLUDE_DIRS}
)
set(HUMID_HTMLVIEW_SOURCES
  src/editorhtmlview.cpp
  src/htmlview_container.cpp
)
set(HUMID_HTMLVIEW_DEFS HUMID_WITH_HTMLVIEW=1)

message(STATUS "HTMLVIEW: enabled (litehtml + Cairo/Pango)")
