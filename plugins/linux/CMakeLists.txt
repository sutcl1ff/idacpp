# linux — extension plugin for idacpp.
#
# Preloads POSIX/Linux system headers (elf.h, sys/mman.h, dlfcn.h, etc.)
# into the REPL so users can reference ELF structures and POSIX APIs
# when analyzing Linux binaries.
#
# Enable: -DPLUGIN_LINUX=ON
# Only meaningful on Linux builds.

include("${CMAKE_CURRENT_SOURCE_DIR}/../common/idacpp_plugin_helpers.cmake")

if(NOT UNIX OR APPLE)
    message(STATUS "linux plugin: skipped (not a Linux build)")
    return()
endif()

message(STATUS "linux plugin: enabled")

# ── PCH bridge ───────────────────────────────────────────────────────────
# Undo pro.h poison macros, then include POSIX/Linux headers.
idacpp_plugin_generate_pch_bridge(
    "${CMAKE_CURRENT_BINARY_DIR}/linux_pch_bridge.h"
    UNDEFS snprintf sprintf getenv
    HEADERS
        elf.h
        link.h
        dlfcn.h
        unistd.h
        fcntl.h
        signal.h
        sys/mman.h
        sys/stat.h
        sys/types.h
        sys/wait.h
        dirent.h)

idacpp_plugin_extend_pch(
    FLAGS "-I${CMAKE_CURRENT_BINARY_DIR}"
    HEADERS linux_pch_bridge.h)

# ── Setup source ─────────────────────────────────────────────────────────
add_library(linux_plugin_init OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/linux_plugin_setup.cpp")
target_link_libraries(linux_plugin_init PRIVATE clinglite)

# ── Register ─────────────────────────────────────────────────────────────
idacpp_register_plugin(linux
    linux_plugin_setup
    "${CMAKE_CURRENT_SOURCE_DIR}/linux_plugin_setup.h"
    "$<TARGET_OBJECTS:linux_plugin_init>")
