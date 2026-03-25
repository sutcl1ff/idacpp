# <PLUGIN_NAME> — extension plugin for idacpp.
#
# Enable: -DPLUGIN_<PLUGIN_NAME>_SRC_DIR=<path>  (auto-enables the plugin)
#   or:   -DPLUGIN_<PLUGIN_NAME>=ON  (for plugins without an external source dir)
#
# Copy this template to plugins/<your-name>/ and replace all <PLUGIN_NAME>
# and <plugin_name> placeholders with your plugin's name.

include("${CMAKE_CURRENT_SOURCE_DIR}/../common/idacpp_plugin_helpers.cmake")

# ── Validate inputs ──────────────────────────────────────────────────────
# Uncomment and adapt if your plugin needs an external source directory:
#
# if(NOT PLUGIN_<PLUGIN_NAME>_SRC_DIR)
#     message(FATAL_ERROR
#         "PLUGIN_<PLUGIN_NAME> requires -DPLUGIN_<PLUGIN_NAME>_SRC_DIR=<path>")
# endif()
# set(_inc "${PLUGIN_<PLUGIN_NAME>_SRC_DIR}/include")

# ── Build library (if your plugin provides one) ──────────────────────────
# Uncomment and adapt for plugins that build a library:
#
# file(GLOB_RECURSE _srcs CONFIGURE_DEPENDS
#     "${PLUGIN_<PLUGIN_NAME>_SRC_DIR}/src/*.cpp")
# add_library(<plugin_name>_plugin_static STATIC ${_srcs})
# target_include_directories(<plugin_name>_plugin_static PUBLIC "${_inc}")
# target_link_libraries(<plugin_name>_plugin_static PUBLIC idasdk_headers)
#
# idacpp_plugin_make_shared(<plugin_name>_plugin_shared <plugin_name>_plugin_static
#     LINK_LIBS idasdk::idalib)

# ── PCH bridge (optional) ────────────────────────────────────────────────
# Generate a bridge header if your plugin headers need #undef of pro.h
# macros before inclusion:
#
# idacpp_plugin_generate_pch_bridge(
#     "${CMAKE_CURRENT_BINARY_DIR}/<plugin_name>_pch_bridge.h"
#     UNDEFS snprintf sprintf getenv
#     HEADERS <your/master_header.hpp>)
#
# idacpp_plugin_extend_pch(
#     FLAGS "-I${_inc}" "-I${CMAKE_CURRENT_BINARY_DIR}"
#     HEADERS <plugin_name>_pch_bridge.h)

# ── Setup source ─────────────────────────────────────────────────────────
# Build the setup .cpp as an OBJECT library. Bake paths via compile defs.
#
# add_library(<plugin_name>_plugin_init OBJECT
#     "${CMAKE_CURRENT_SOURCE_DIR}/<plugin_name>_plugin_setup.cpp")
# target_link_libraries(<plugin_name>_plugin_init PRIVATE clinglite)
# target_compile_definitions(<plugin_name>_plugin_init PRIVATE
#     PLUGIN_<PLUGIN_NAME>_INCLUDE_DIR="${_inc}"
#     PLUGIN_<PLUGIN_NAME>_SHARED_LIB="$<TARGET_FILE:<plugin_name>_plugin_shared>")

# ── Register ─────────────────────────────────────────────────────────────
# idacpp_register_plugin(<plugin_name>
#     <plugin_name>_plugin_setup
#     "${CMAKE_CURRENT_SOURCE_DIR}/<plugin_name>_plugin_setup.h"
#     "$<TARGET_OBJECTS:<plugin_name>_plugin_init>")
