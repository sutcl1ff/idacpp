# idax — extension plugin for idacpp.
#
# Provides the idax C++23 IDA SDK wrapper as a runtime-loadable library.
# When enabled, the REPL preloads idax headers and shared library so users
# can immediately use ida::function::all(), ida::segment::count(), etc.
#
# Enable: -DPLUGIN_IDAX_SRC_DIR=<path-to-idax-source>

include("${CMAKE_CURRENT_SOURCE_DIR}/../common/idacpp_plugin_helpers.cmake")

# Validate inputs
if(NOT PLUGIN_IDAX_SRC_DIR)
    message(FATAL_ERROR
        "idax plugin requires -DPLUGIN_IDAX_SRC_DIR=<path-to-idax>\n"
        "  Point it at the idax source tree containing include/ida/idax.hpp")
endif()

file(TO_CMAKE_PATH "${PLUGIN_IDAX_SRC_DIR}" PLUGIN_IDAX_SRC_DIR)
set(_idax_inc "${PLUGIN_IDAX_SRC_DIR}/include")
if(NOT EXISTS "${_idax_inc}/ida/idax.hpp")
    message(FATAL_ERROR
        "idax master header not found at ${_idax_inc}/ida/idax.hpp\n"
        "  Check PLUGIN_IDAX_SRC_DIR=${PLUGIN_IDAX_SRC_DIR}")
endif()

message(STATUS "idax plugin: ${PLUGIN_IDAX_SRC_DIR}")

# Find Python (needed by gen_exports_def.py for shared lib .def generation)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Ensure idasdk_headers target exists
# idax expects an idasdk_headers INTERFACE target providing SDK includes.
# When built standalone via idax's own CMakeLists.txt it creates this target,
# but here we build idax sources directly, so create it if missing.
if(NOT TARGET idasdk_headers)
    add_library(idasdk_headers INTERFACE IMPORTED)
    set_target_properties(idasdk_headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$ENV{IDASDK}/include")
    target_compile_definitions(idasdk_headers INTERFACE __EA64__)
    target_link_libraries(idasdk_headers INTERFACE
        ida_platform_settings
        ida_compiler_settings)
endif()

# Build idax static library
file(GLOB_RECURSE _idax_srcs CONFIGURE_DEPENDS
    "${PLUGIN_IDAX_SRC_DIR}/src/*.cpp")

add_library(idax_plugin_static STATIC ${_idax_srcs})
target_include_directories(idax_plugin_static
    PUBLIC "${_idax_inc}"
    PRIVATE "${PLUGIN_IDAX_SRC_DIR}/src")
target_link_libraries(idax_plugin_static PUBLIC idasdk_headers)
target_compile_features(idax_plugin_static PUBLIC cxx_std_23)
set_property(TARGET idax_plugin_static
    PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)

if(MSVC)
    target_compile_options(idax_plugin_static PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(idax_plugin_static PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Shared variant for JIT loading
clinglite_plugin_make_shared(idax_plugin_shared idax_plugin_static
    LINK_LIBS idasdk::idalib)

# PCH bridge (undoes pro.h poison, then includes idax)
clinglite_plugin_generate_pch_bridge(
    "${CMAKE_CURRENT_BINARY_DIR}/idax_pch_bridge.h"
    UNDEFS snprintf sprintf getenv
    HEADERS ida/idax.hpp)

idacpp_plugin_extend_pch(
    FLAGS "-I${_idax_inc}" "-I${CMAKE_CURRENT_BINARY_DIR}"
    HEADERS idax_pch_bridge.h)

# Setup source (baked paths via compile definitions)
add_library(idax_plugin_init OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/idax_plugin_setup.cpp")
target_link_libraries(idax_plugin_init PRIVATE clinglite)
target_compile_definitions(idax_plugin_init PRIVATE
    IDACPP_PLUGIN_IDAX_INCLUDE_DIR="${_idax_inc}"
    IDACPP_PLUGIN_IDAX_SHARED_LIB="$<TARGET_FILE:idax_plugin_shared>")
add_dependencies(idax_plugin_init idax_plugin_shared)

if(MSVC)
    target_compile_options(idax_plugin_init PRIVATE /EHsc)
endif()

# Register with dispatch system
idacpp_register_plugin(idax
    idax_plugin_setup
    "${CMAKE_CURRENT_SOURCE_DIR}/idax_plugin_setup.h"
    "$<TARGET_OBJECTS:idax_plugin_init>")
