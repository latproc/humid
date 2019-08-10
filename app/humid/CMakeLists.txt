cmake_minimum_required(VERSION 2.80)

if(MINGW_BUILD)
else()
    if (RUN_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

set(exe_name "humid")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LocalCMakeLists.txt")
    include("${CMAKE_CURRENT_SOURCE_DIR}/LocalCMakeLists.txt")
endif()

#################################
##        find library         ##
#################################

set(INCLUDE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

if (MINGW_BUILD)
    set(LIBRARY
        clockwork_client
        nanogui
        opengl32
        ws2_32
    )
else()
    set(LIBRARY
        clockwork_client
        nanogui
    )
endif()

##################################
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

##################################
#  generate yacc and bison files #
##################################

find_package(FLEX)
if (NOT FLEX_EXECUTABLE)
     message(FATAL_ERROR "flex was not found.")
endif()

find_package(BISON)
if (NOT BISON_EXECUTABLE)
    message(FATAL_ERROR "bison not found.")
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")


set(cw_set_lex_input ${CMAKE_CURRENT_SOURCE_DIR}/src_generators/settingslang.lpp)
set(cw_set_yac_input ${CMAKE_CURRENT_SOURCE_DIR}/src_generators/settingslang.ypp)
set(cw_hmi_lex_input ${CMAKE_CURRENT_SOURCE_DIR}/src_generators/hmilang.lpp)
set(cw_hmi_yac_input ${CMAKE_CURRENT_SOURCE_DIR}/src_generators/hmilang.ypp)

set(cw_set_lex_output ${CMAKE_CURRENT_BINARY_DIR}/generated/settingslang.yy.cpp)
set(cw_set_yacc_output ${CMAKE_CURRENT_BINARY_DIR}/generated/settingslang.tab.cpp)
set(cw_hmi_lex_output ${CMAKE_CURRENT_BINARY_DIR}/generated/hmilang.yy.cpp)
set(cw_hmi_yacc_output ${CMAKE_CURRENT_BINARY_DIR}/generated/hmilang.tab.cpp)

ADD_CUSTOM_COMMAND(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS -o ${cw_set_lex_output}
    	${cw_set_lex_input}
    OUTPUT ${cw_set_lex_output}
)
ADD_CUSTOM_COMMAND(
    COMMAND ${BISON_EXECUTABLE}
    ARGS -y
        -o ${cw_set_yacc_output}
    	-g -v -d ${cw_set_yac_input}
    DEPENDS ${cw_set_lex_input}
    OUTPUT ${cw_set_yacc_output}
)
##
ADD_CUSTOM_COMMAND(
    COMMAND ${FLEX_EXECUTABLE}
    ARGS -o ${cw_hmi_lex_output}
    	${cw_hmi_lex_input}
    OUTPUT ${cw_hmi_lex_output}
)

ADD_CUSTOM_COMMAND(
    COMMAND ${BISON_EXECUTABLE}
    ARGS -y
        -o ${cw_hmi_yacc_output}
    	-g -v -d ${cw_hmi_yac_input}
    DEPENDS ${cw_hmi_lex_input}
    OUTPUT ${cw_hmi_yacc_output}
)

# Since the parser source does not exists yet when cmake is run, mark
# it as generated
SET_SOURCE_FILES_PROPERTIES(${cw_hmi_lex_output} GENERATED)
SET_SOURCE_FILES_PROPERTIES(${cw_hmi_yacc_output} GENERATED)
SET_SOURCE_FILES_PROPERTIES(${cw_set_lex_output} GENERATED)
SET_SOURCE_FILES_PROPERTIES(${cw_set_yacc_output} GENERATED)




##################################
#          Build Binary          #
##################################

FILE(GLOB_RECURSE source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
)
FILE(GLOB_RECURSE header_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
)
list(APPEND source_files
    ${cw_hmi_lex_output}
    ${cw_hmi_yacc_output}
    ${cw_set_lex_output}
    ${cw_set_yacc_output}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# add_library(${exe_name}
#     STATIC
#     ${source_files}
#     ${header_files}
# )
add_executable(${exe_name} ${source_files} ${header_files})
# add_test(${exe_name} ${exe_name})

target_link_libraries(${exe_name}
    ${LIBRARY}
)
target_include_directories(${exe_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/humid
    ${INCLUDE}
)

# if (MINGW_BUILD)
# else()
#     install(
#         TARGETS
#         ${exe_name}
#         LIBRARY DESTINATION
#         ${PROJECT_SOURCE_DIR}
#         ARCHIVE DESTINATION
#         ${PROJECT_SOURCE_DIR}
#     )
# endif()
