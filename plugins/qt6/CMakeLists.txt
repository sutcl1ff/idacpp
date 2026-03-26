# qt6 — extension plugin for idacpp.
#
# Provides Qt6 (Core, Gui, Widgets) support in the C++ REPL. Discovers an
# existing Qt6 installation built with QT_NAMESPACE=QT, or offers to build
# one via the IDA SDK's QtSupport.cmake.
#
# Enable: -DPLUGIN_QT6=ON [-DPLUGIN_QT6_DIR=<path-to-qt6-install>]

include("${CMAKE_CURRENT_SOURCE_DIR}/../common/idacpp_plugin_helpers.cmake")

# Qt6 discovery (layered, first match wins)
# (a) Explicit override via -DPLUGIN_QT6_DIR
if(PLUGIN_QT6_DIR)
    file(TO_CMAKE_PATH "${PLUGIN_QT6_DIR}" PLUGIN_QT6_DIR)
    list(PREPEND CMAKE_PREFIX_PATH "${PLUGIN_QT6_DIR}")
    message(STATUS "qt6 plugin: using PLUGIN_QT6_DIR=${PLUGIN_QT6_DIR}")
endif()

# (b) IDA SDK's QtSupport.cmake — auto-detects ${CMAKE_BINARY_DIR}/qt-install/
#     and creates the build_qt target for first-time Qt builds.
set(_qt_support "$ENV{IDASDK}/src/cmake/cmake/QtSupport.cmake")
if(EXISTS "${_qt_support}" AND NOT TARGET build_qt)
    include("${_qt_support}")
    message(STATUS "qt6 plugin: included IDA SDK QtSupport.cmake")
endif()

# (c) Try to find Qt6
find_package(Qt6 QUIET COMPONENTS Core Gui Widgets)

# (d) Fail or soft-fail depending on whether an explicit override was given
if(NOT Qt6_FOUND)
    if(PLUGIN_QT6_DIR)
        message(FATAL_ERROR
            "qt6 plugin: Qt6 not found at PLUGIN_QT6_DIR=${PLUGIN_QT6_DIR}")
    elseif(TARGET build_qt)
        message(STATUS "qt6 plugin: Qt6 not found — build it with:")
        message(STATUS "  cmake --build ${CMAKE_BINARY_DIR} --target build_qt")
    else()
        message(STATUS "qt6 plugin: Qt6 not found, skipping")
        message(STATUS "  Set -DPLUGIN_QT6_DIR=<path-to-qt6-install>")
    endif()
    return()
endif()

# Validate found Qt6
# If PLUGIN_QT6_DIR was set, the resolved Qt6 must come from that prefix.
if(PLUGIN_QT6_DIR)
    file(REAL_PATH "${PLUGIN_QT6_DIR}" _qt6_override_real)
    file(REAL_PATH "${Qt6_DIR}" _qt6_found_real)
    string(FIND "${_qt6_found_real}" "${_qt6_override_real}/" _pos)
    if(NOT _pos EQUAL 0)
        message(FATAL_ERROR
            "qt6 plugin: Qt6 found at ${Qt6_DIR} which is outside PLUGIN_QT6_DIR=${PLUGIN_QT6_DIR}\n"
            "  Resolved override: ${_qt6_override_real}\n"
            "  Resolved Qt6_DIR:  ${_qt6_found_real}\n"
            "  Remove stale Qt from CMAKE_PREFIX_PATH or fix PLUGIN_QT6_DIR")
    endif()
endif()

# IDA's Qt is built with QT_NAMESPACE=QT — reject incompatible installs.
get_target_property(_qt6_core_defs Qt6::Core INTERFACE_COMPILE_DEFINITIONS)
if(NOT _qt6_core_defs MATCHES "QT_NAMESPACE=QT")
    message(FATAL_ERROR
        "qt6 plugin: Qt6 at ${Qt6_DIR} was not built with QT_NAMESPACE=QT\n"
        "  IDA requires QT_NAMESPACE=QT. Build Qt with: -DQT_NAMESPACE=QT")
endif()

message(STATUS "qt6 plugin: Qt6 ${Qt6_VERSION} found")

# Extract Qt6 paths
# Derive the Qt install prefix from Qt6_DIR (set by find_package).
# Qt6_DIR is typically <prefix>/lib/cmake/Qt6, so go up 3 levels.
get_filename_component(_qt6_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(_qt6_inc_main "${_qt6_prefix}/include")

# Collect all include dirs from imported targets for PCH flags.
set(_qt6_all_inc_dirs "${_qt6_inc_main}")
foreach(_comp Core Gui Widgets)
    get_target_property(_dirs Qt6::${_comp} INTERFACE_INCLUDE_DIRECTORIES)
    if(_dirs)
        list(APPEND _qt6_all_inc_dirs ${_dirs})
    endif()
endforeach()
list(REMOVE_DUPLICATES _qt6_all_inc_dirs)

# Resolve Qt6 runtime libraries from IDABIN
# Use IDA's copies (what IDA actually loads at runtime), not qt-install.
# Resolve actual filenames via glob to handle versioned names (e.g. .so.6).
if(NOT DEFINED IDABIN)
    set(IDABIN "${IDASDK}/bin")
endif()

function(_qt6_resolve_runtime var component)
    if(WIN32)
        set(_pattern "${IDABIN}/${component}.dll")
    elseif(APPLE)
        set(_pattern "${IDABIN}/lib${component}*.dylib")
    else()
        set(_pattern "${IDABIN}/lib${component}.so*")
    endif()
    file(GLOB _matches "${_pattern}")
    if(NOT _matches)
        message(FATAL_ERROR
            "qt6 plugin: ${component} runtime not found in ${IDABIN}\n"
            "  Pattern: ${_pattern}")
    endif()
    list(GET _matches 0 _resolved)
    set(${var} "${_resolved}" PARENT_SCOPE)
endfunction()

_qt6_resolve_runtime(_qt6_core_lib Qt6Core)
_qt6_resolve_runtime(_qt6_gui_lib Qt6Gui)
_qt6_resolve_runtime(_qt6_widgets_lib Qt6Widgets)

message(STATUS "qt6 plugin: include=${_qt6_inc_main}")
message(STATUS "qt6 plugin: core=${_qt6_core_lib}")
message(STATUS "qt6 plugin: gui=${_qt6_gui_lib}")
message(STATUS "qt6 plugin: widgets=${_qt6_widgets_lib}")

# PCH bridge (undoes pro.h poison, defines Qt namespace, includes Qt headers)
clinglite_plugin_generate_pch_bridge(
    "${CMAKE_CURRENT_BINARY_DIR}/qt6_pch_bridge.h"
    UNDEFS snprintf sprintf getenv
    DEFINES QT_NAMESPACE=QT QT_CORE_LIB QT_GUI_LIB QT_WIDGETS_LIB QT_DLL
    HEADERS QtCore/QtCore QtGui/QtGui QtWidgets/QtWidgets)

# Build PCH flags from all Qt include directories
set(_qt6_pch_flags)
foreach(_dir ${_qt6_all_inc_dirs})
    list(APPEND _qt6_pch_flags "-I${_dir}")
endforeach()

idacpp_plugin_extend_pch(
    FLAGS
        ${_qt6_pch_flags}
        "-I${CMAKE_CURRENT_BINARY_DIR}"
        "-DQT_NAMESPACE=QT"
        "-DQT_CORE_LIB"
        "-DQT_GUI_LIB"
        "-DQT_WIDGETS_LIB"
        "-DQT_DLL"
    HEADERS qt6_pch_bridge.h)

# Setup source (baked paths via compile definitions)
add_library(qt6_plugin_init OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/qt6_plugin_setup.cpp")
target_link_libraries(qt6_plugin_init PRIVATE clinglite)
target_compile_definitions(qt6_plugin_init PRIVATE
    IDACPP_PLUGIN_QT6_INCLUDE_DIR="${_qt6_inc_main}"
    IDACPP_PLUGIN_QT6_CORE_LIB="${_qt6_core_lib}"
    IDACPP_PLUGIN_QT6_GUI_LIB="${_qt6_gui_lib}"
    IDACPP_PLUGIN_QT6_WIDGETS_LIB="${_qt6_widgets_lib}")

if(MSVC)
    target_compile_options(qt6_plugin_init PRIVATE /EHsc)
endif()

# Register with dispatch system
idacpp_register_plugin(qt6
    qt6_plugin_setup
    "${CMAKE_CURRENT_SOURCE_DIR}/qt6_plugin_setup.h"
    "$<TARGET_OBJECTS:qt6_plugin_init>")
