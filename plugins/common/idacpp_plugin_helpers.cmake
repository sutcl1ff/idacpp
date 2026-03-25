# idacpp_plugin_helpers.cmake — shared cmake functions for all idacpp plugins.
#
# Provides:
#   idacpp_register_plugin()         — register a setup function with dispatch
#   idacpp_plugin_extend_pch()       — add headers/flags to the shared PCH
#   idacpp_plugin_generate_pch_bridge() — generate #undef + #include bridge header
#   idacpp_plugin_make_shared()      — wrap a static lib as shared for JIT loading

# ── Plugin registration ──────────────────────────────────────────────────
# Each extension plugin calls this to register its setup function.
# The top-level plugins/CMakeLists.txt generates a dispatch .cpp from the
# collected functions.
#
# Arguments:
#   PLUGIN_NAME   — short name (e.g. "idax")
#   SETUP_FN      — C++ function name (e.g. "idax_plugin_setup")
#   SETUP_HEADER  — absolute path to the header declaring SETUP_FN
#   SETUP_SOURCE  — source/object to add to idacpp_core (path or generator expr)
function(idacpp_register_plugin PLUGIN_NAME SETUP_FN SETUP_HEADER SETUP_SOURCE)
    set(IDACPP_PLUGIN_SETUP_FUNCTIONS
        ${IDACPP_PLUGIN_SETUP_FUNCTIONS} "${SETUP_FN}" CACHE INTERNAL "")
    set(IDACPP_PLUGIN_SETUP_HEADERS
        ${IDACPP_PLUGIN_SETUP_HEADERS} "${SETUP_HEADER}" CACHE INTERNAL "")
    set(IDACPP_PLUGIN_SOURCES
        ${IDACPP_PLUGIN_SOURCES} "${SETUP_SOURCE}" CACHE INTERNAL "")
    set(IDACPP_PLUGIN_NAMES
        ${IDACPP_PLUGIN_NAMES} "${PLUGIN_NAME}" CACHE INTERNAL "")
endfunction()

# ── PCH contribution ─────────────────────────────────────────────────────
# Any plugin can add include flags and headers to the shared PCH.
# Headers are appended in order: ida_sdk first, then extensions.
#
# Named arguments:
#   FLAGS   — extra -I or -D flags for clinglite_pchgen
#   HEADERS — header file names to include (after IDA SDK headers)
function(idacpp_plugin_extend_pch)
    cmake_parse_arguments(ARG "" "" "FLAGS;HEADERS" ${ARGN})
    set(IDACPP_PLUGIN_PCH_FLAGS
        ${IDACPP_PLUGIN_PCH_FLAGS} ${ARG_FLAGS} CACHE INTERNAL "")
    set(IDACPP_PLUGIN_PCH_HEADERS
        ${IDACPP_PLUGIN_PCH_HEADERS} ${ARG_HEADERS} CACHE INTERNAL "")
endfunction()

# ── PCH bridge generation ────────────────────────────────────────────────
# Generates a header file that undefs macros then includes plugin headers.
# Common pattern for plugins that layer on top of pro.h.
#
# Arguments:
#   OUTPUT_FILE — absolute path for the generated header
# Named arguments:
#   UNDEFS  — macro names to #undef (e.g. snprintf sprintf getenv)
#   HEADERS — headers to #include <...> after the undefs
function(idacpp_plugin_generate_pch_bridge OUTPUT_FILE)
    cmake_parse_arguments(ARG "" "" "UNDEFS;HEADERS" ${ARGN})
    set(_content "// Auto-generated PCH bridge for idacpp plugin\n")
    foreach(_undef ${ARG_UNDEFS})
        string(APPEND _content "#undef ${_undef}\n")
    endforeach()
    foreach(_hdr ${ARG_HEADERS})
        string(APPEND _content "#include <${_hdr}>\n")
    endforeach()
    file(WRITE "${OUTPUT_FILE}" "${_content}")
endfunction()

# ── Shared library from static archive ───────────────────────────────────
# Wraps a static lib as a shared lib (WHOLE_ARCHIVE) for JIT loading.
#
# Arguments:
#   TARGET_NAME — name for the shared library target
#   STATIC_LIB  — static library target to wrap
# Named arguments:
#   LINK_LIBS — additional libraries to link (e.g. idasdk::idalib)
function(idacpp_plugin_make_shared TARGET_NAME STATIC_LIB)
    cmake_parse_arguments(ARG "" "" "LINK_LIBS" ${ARGN})
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_stub.cpp" "")
    if(MSVC)
        # IDA SDK forces /GL + /LTCG, so .obj files are LTCG bitcode and
        # CMake's WINDOWS_EXPORT_ALL_SYMBOLS (which uses __create_def to
        # parse COFF .obj) cannot work. Generate a .def from the static
        # .lib via dumpbin and add it as a source so MSVC links with it.
        set(_gen_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/gen_exports_def.py")
        set(_def "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_all.def")
        add_custom_command(
            OUTPUT "${_def}"
            COMMAND ${Python3_EXECUTABLE} "${_gen_script}"
                "$<TARGET_FILE:${STATIC_LIB}>" "${_def}"
            DEPENDS ${STATIC_LIB} "${_gen_script}"
            COMMENT "Generating ${TARGET_NAME} export definitions"
            VERBATIM)
        # Add .def as a source — MSVC linker picks it up automatically
        add_library(${TARGET_NAME} SHARED
            "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_stub.cpp"
            "${_def}")
    else()
        add_library(${TARGET_NAME} SHARED
            "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_stub.cpp")
    endif()
    target_link_libraries(${TARGET_NAME} PRIVATE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,${STATIC_LIB}>")
    foreach(_lib ${ARG_LINK_LIBS})
        target_link_libraries(${TARGET_NAME} PRIVATE ${_lib})
    endforeach()
endfunction()
