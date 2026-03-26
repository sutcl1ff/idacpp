# idacpp plugin system — discovery, dispatch generation, PCH assembly, idgen.
#
# Orchestrates IDA-specific plugins:
#   1. ida_sdk (built-in, always-on) — provides IDA SDK header list
#   2. Extension plugins (opt-in via -DPLUGIN_<NAME>_SRC_DIR=<path> or -DPLUGIN_<NAME>=ON)
#   3. Generates plugin dispatch .cpp (reuses clinglite_generate_plugin_dispatch)
#   4. Assembles shared PCH from all plugin contributions (IDA + clinglite)
#   5. Generates identifier table for tab completion

include("${CMAKE_CURRENT_SOURCE_DIR}/common/idacpp_plugin_helpers.cmake")

# Accumulators (reset, then plugins append via CACHE INTERNAL)
set(IDACPP_PLUGIN_PCH_FLAGS "" CACHE INTERNAL "")
set(IDACPP_PLUGIN_PCH_HEADERS "" CACHE INTERNAL "")
set(IDACPP_PLUGIN_SOURCES "" CACHE INTERNAL "")
set(IDACPP_PLUGIN_SETUP_FUNCTIONS "" CACHE INTERNAL "")
set(IDACPP_PLUGIN_SETUP_HEADERS "" CACHE INTERNAL "")
set(IDACPP_PLUGIN_NAMES "" CACHE INTERNAL "")

# Built-in plugins (always enabled)
add_subdirectory(ida_sdk)

# Extension plugins (opt-in, auto-enabled by _SRC_DIR or explicit =ON)
# Scans plugins/<name>/CMakeLists.txt, skipping built-in and common.
set(_skip_dirs "ida_sdk" "common")
file(GLOB _plugin_dirs "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
foreach(_plugin_cmake ${_plugin_dirs})
    get_filename_component(_plugin_dir "${_plugin_cmake}" DIRECTORY)
    get_filename_component(_plugin_name "${_plugin_dir}" NAME)
    if(_plugin_name IN_LIST _skip_dirs)
        continue()
    endif()
    string(TOUPPER "${_plugin_name}" _PLUGIN_NAME)
    # Auto-enable if a source directory was provided (no need for -DPLUGIN_<NAME>=ON)
    if(DEFINED PLUGIN_${_PLUGIN_NAME}_SRC_DIR AND NOT DEFINED PLUGIN_${_PLUGIN_NAME})
        set(PLUGIN_${_PLUGIN_NAME} ON CACHE BOOL "Auto-enabled via PLUGIN_${_PLUGIN_NAME}_SRC_DIR" FORCE)
    endif()
    if(PLUGIN_${_PLUGIN_NAME})
        message(STATUS "idacpp plugin: ${_plugin_name}")
        add_subdirectory("${_plugin_dir}"
            "${CMAKE_CURRENT_BINARY_DIR}/plugins/${_plugin_name}")
    endif()
endforeach()

# Generate plugin dispatch .cpp
# Reuses clinglite's parameterized dispatch generator with IDACPP prefix.
clinglite_generate_plugin_dispatch(IDACPP "idacpp::plugins"
    "${CMAKE_CURRENT_BINARY_DIR}/plugin_dispatch.cpp")

# Wire into idacpp_core
target_sources(idacpp_core PRIVATE
    ${IDACPP_PLUGIN_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/plugin_dispatch.cpp")
target_include_directories(idacpp_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/common")

# Build-time PCH assembly (shared service)
# Assembles one PCH from all plugin contributions:
#   IDA_PCH_HEADERS             — ida_sdk base headers (from ida_sdk plugin)
#   CLINGLITE_PLUGIN_PCH_FLAGS  — flags from clinglite plugins (linux, winsdk)
#   CLINGLITE_PLUGIN_PCH_HEADERS — headers from clinglite plugins
#   IDACPP_PLUGIN_PCH_FLAGS     — extra flags from idacpp plugins (qt6, idax)
#   IDACPP_PLUGIN_PCH_HEADERS   — extra headers from idacpp plugins
if(IDA_PCH_HEADERS AND TARGET clinglite_pchgen)
    set(IDA_PCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/ida_sdk.pch")

    add_custom_command(
        OUTPUT ${IDA_PCH_FILE}
        COMMAND clinglite_pchgen
            -I$ENV{IDASDK}/include
            ${CLINGLITE_PLUGIN_PCH_FLAGS}
            ${IDACPP_PLUGIN_PCH_FLAGS}
            -D__EA64__=1 -D__IDP__ -D${IDAPROPLAT}
            -std=c++2b
            -o ${IDA_PCH_FILE}
            ${IDA_PCH_HEADERS}
            ${CLINGLITE_PLUGIN_PCH_HEADERS}
            ${IDACPP_PLUGIN_PCH_HEADERS}
        DEPENDS clinglite_pchgen
            "${CMAKE_CURRENT_SOURCE_DIR}/ida_sdk/ida_sdk.h"
        COMMENT "Generating PCH (IDA SDK + clinglite plugins + idacpp plugins)"
    )
    add_custom_target(ida_pch DEPENDS ${IDA_PCH_FILE})
else()
    message(WARNING
        "PCH generation disabled — clinglite_pchgen not available or no headers")
endif()

# Identifier table generation
# Uses idacpp_idgen (built in parent cmake) to scan IDA SDK headers and
# shared libraries for identifiers used in tab completion.
set(IDA_IDGEN_INC "${CMAKE_CURRENT_BINARY_DIR}/ida_sdk_identifiers.inc")

# Find IDA shared libraries for export extraction
set(_ida_libs)
foreach(_lib libida.so libidalib.so ida.dll idalib.dll libida.dylib libidalib.dylib)
    if(EXISTS "$ENV{IDASDK}/bin/${_lib}")
        list(APPEND _ida_libs "$ENV{IDASDK}/bin/${_lib}")
    endif()
endforeach()

if(IDA_PCH_HEADERS AND TARGET idacpp_idgen)
    set(_idgen_args
        -I$ENV{IDASDK}/include
        -D__EA64__=1 -D__IDP__ -D${IDAPROPLAT}
        -std=c++2b
        --sdk-dir $ENV{IDASDK}/include
        -o ${IDA_IDGEN_INC}
        ${IDA_PCH_HEADERS})
    if(_ida_libs)
        list(APPEND _idgen_args --libs ${_ida_libs})
    endif()

    add_custom_command(
        OUTPUT ${IDA_IDGEN_INC}
        COMMAND idacpp_idgen ${_idgen_args}
        DEPENDS idacpp_idgen
            "${CMAKE_CURRENT_SOURCE_DIR}/ida_sdk/ida_sdk.h"
        COMMENT "Generating IDA SDK identifier table"
    )
    add_custom_target(ida_identifiers DEPENDS ${IDA_IDGEN_INC})

    # Wire identifier table into idacpp_core and idacpp_plugin
    add_dependencies(idacpp_core ida_identifiers)
    target_include_directories(idacpp_core PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}")
    add_dependencies(idacpp_plugin ida_identifiers)
    target_include_directories(idacpp_plugin PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}")
endif()
