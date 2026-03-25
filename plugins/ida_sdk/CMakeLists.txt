# ida_sdk — built-in plugin. Always enabled, loads first.
#
# Owns the IDA SDK header list used for PCH generation and identifier table
# generation. The header inventory is parsed from ida_sdk.h in this directory.
#
# Runtime setup (library loading, PCH caching, extlang registration) is
# handled by the existing idalib_setup.cpp — this plugin only provides the
# build-time configuration.

# Parse ida_sdk.h — extract individual header names for pchgen + idgen
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/ida_sdk.h" _ida_hdr_lines
    REGEX "^#include")
set(IDA_PCH_HEADERS)
foreach(_line ${_ida_hdr_lines})
    string(REGEX REPLACE "^#include [<\"]([^>\"]+)[>\"].*" "\\1" _hdr "${_line}")
    list(APPEND IDA_PCH_HEADERS "${_hdr}")
endforeach()

# Export to parent — plugins/CMakeLists.txt uses this for PCH + idgen
set(IDA_PCH_HEADERS "${IDA_PCH_HEADERS}" PARENT_SCOPE)

list(LENGTH IDA_PCH_HEADERS _count)
message(STATUS "ida_sdk plugin: ${_count} headers from ida_sdk.h")
