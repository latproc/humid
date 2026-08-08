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
if(PKG_CONFIG_EXECUTABLE OR PkgConfig_FOUND OR PKG_CONFIG_FOUND)
  find_package(PkgConfig QUIET)
  if(COMMAND pkg_check_modules)
    pkg_check_modules(HUMID_CAIRO QUIET cairo pangocairo fontconfig)
    if(HUMID_CAIRO_FOUND)
      set(_HUMID_HAVE_CAIRO TRUE)
    endif()
  endif()
endif()

if(NOT _HUMID_HAVE_CAIRO)
  message(WARNING "HTMLVIEW: Cairo/Pango not found; building without HTMLVIEW. Install cairo/pangocairo or set -DHUMID_WITH_HTMLVIEW=OFF")
  set(HUMID_WITH_HTMLVIEW OFF CACHE BOOL "Build embedded HTMLVIEW (litehtml + Cairo/Pango)" FORCE)
  return()
endif()

set(LITEHTML_ROOT "${PROJECT_SOURCE_DIR}/lib/litehtml")
if(NOT EXISTS "${LITEHTML_ROOT}/include/litehtml.h")
  message(WARNING "HTMLVIEW: lib/litehtml missing; building without HTMLVIEW")
  set(HUMID_WITH_HTMLVIEW OFF CACHE BOOL "Build embedded HTMLVIEW (litehtml + Cairo/Pango)" FORCE)
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
