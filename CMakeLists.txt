cmake_minimum_required(VERSION 3.20)
project(idacpp VERSION 0.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── CRT policy: static CRT (/MT) to match LLVM and produce self-contained DLLs
# CMP0091 NEW enables the MSVC_RUNTIME_LIBRARY target property so we can
# force /MT for all configs (including Debug) to match the LLVM libs.
# Must be set before add_subdirectory(clinglite) so clinglite targets inherit.
if(MSVC)
  cmake_policy(SET CMP0091 NEW)
  # CACHE FORCE so clinglite's own CACHE set (without FORCE) doesn't override.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING
    "MSVC runtime library — /MT for fully static plugin binary" FORCE)
endif()

# ── Locate clinglite source directory ─────────────────────────────────────
# Priority: 1) -DCLINGLITE_SOURCE_DIR  2) env CLINGLITE_SOURCE_DIR
if(NOT CLINGLITE_SOURCE_DIR)
  if(DEFINED ENV{CLINGLITE_SOURCE_DIR})
    set(CLINGLITE_SOURCE_DIR "$ENV{CLINGLITE_SOURCE_DIR}")
  else()
    message(FATAL_ERROR
      "clinglite source directory not found. Either:\n"
      "  - Set: -DCLINGLITE_SOURCE_DIR=<path-to-clinglite-source>\n"
      "  - Set: env CLINGLITE_SOURCE_DIR=<path>")
  endif()
endif()
message(STATUS "clinglite source: ${CLINGLITE_SOURCE_DIR}")

# ── Cling build directory (optional pass-through to clinglite) ────────────
if(NOT CLING_BUILD_DIR)
  if(DEFINED ENV{CLING_BUILD_DIR})
    set(CLING_BUILD_DIR "$ENV{CLING_BUILD_DIR}")
  endif()
  # If still unset, clinglite's own CMake will attempt discovery or error.
endif()

# ── clinglite library ──────────────────────────────────────────────────────
# Guard: clinglite may already be a target (monorepo inclusion)
if(NOT TARGET clinglite)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  add_subdirectory("${CLINGLITE_SOURCE_DIR}" clinglite)
endif()

# ── Clang resource dir (propagated from clinglite, with fallback) ─────────
# CLINGLITE_CLANG_BUILTIN_DIR and CLINGLITE_CLANG_RESOURCE_VERSION are
# propagated by clinglite via PARENT_SCOPE.  Fall back to manual derivation
# only when clinglite was already a target before our add_subdirectory().
if(NOT CLINGLITE_CLANG_BUILTIN_DIR AND CLING_BUILD_DIR)
  file(GLOB _clang_res_dirs "${CLING_BUILD_DIR}/lib/clang/*")
  foreach(_dir ${_clang_res_dirs})
    if(IS_DIRECTORY "${_dir}/include")
      get_filename_component(CLINGLITE_CLANG_RESOURCE_VERSION "${_dir}" NAME)
      set(CLINGLITE_CLANG_BUILTIN_DIR "${_dir}/include")
      break()
    endif()
  endforeach()
endif()

# ── IDA SDK (required) ────────────────────────────────────────────────────
# Accept IDASDK as cmake variable (-DIDASDK=...) or env variable
if(DEFINED IDASDK AND NOT DEFINED ENV{IDASDK})
  set(ENV{IDASDK} "${IDASDK}")
elseif(DEFINED ENV{IDASDK} AND NOT DEFINED IDASDK)
  set(IDASDK "$ENV{IDASDK}")
endif()

if(NOT DEFINED IDASDK)
  message(FATAL_ERROR
    "Set -DIDASDK=<path-to-idasdk> or env IDASDK")
endif()

# Guard: IDA SDK may already be bootstrapped (monorepo inclusion)
if(NOT TARGET ida_platform_settings)
  include($ENV{IDASDK}/src/cmake/bootstrap.cmake)
  # Re-sync: bootstrap may adjust $ENV{IDASDK} for GitHub-clone SDK layouts
  set(IDASDK "$ENV{IDASDK}")
  find_package(idasdk REQUIRED)
endif()
message(STATUS "IDA SDK found: $ENV{IDASDK}")

# ── Restore /MT after IDA SDK bootstrap ──────────────────────────────────
# IDA SDK's platform.cmake sets CMAKE_MSVC_RUNTIME_LIBRARY to /MT with a
# Debug genex. Override to unconditional /MT so all configs match the
# Release LLVM static libs. The VS generator adds /D_DEBUG for Debug
# configs, causing the MSVC STL to stamp objects as MTd_StaticDebug
# (LNK2038 with Release LLVM). /U_DEBUG undoes that;
# _ITERATOR_DEBUG_LEVEL=0 matches the LLVM libs.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  add_compile_options($<$<CONFIG:Debug>:/U_DEBUG>)
  add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
endif()

# ── Identifier generator tool ─────────────────────────────────────────────
add_executable(idacpp_idgen tools/idgen.cpp)
target_link_libraries(idacpp_idgen PRIVATE clinglite)
if(MSVC)
  target_compile_options(idacpp_idgen PRIVATE /EHsc)
endif()

# ── Shared extlang library ─────────────────────────────────────────────────
# Static library used by both the plugin and standalone consumers.
add_library(idacpp_core STATIC extlang_core.cpp idalib_setup.cpp cpp_highlighter.cpp)
target_include_directories(idacpp_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(idacpp_core PRIVATE $ENV{IDASDK}/include)
target_link_libraries(idacpp_core PUBLIC clinglite)
target_compile_definitions(idacpp_core PRIVATE
  IDACPP_VERSION="${PROJECT_VERSION}")

if(MSVC)
  target_compile_options(idacpp_core PRIVATE /EHsc)
endif()

# ── Plugin target ─────────────────────────────────────────────────────────
ida_add_plugin(idacpp_plugin
  SOURCES plugin.cpp
  LIBRARIES idacpp_core)
set_target_properties(idacpp_plugin PROPERTIES OUTPUT_NAME idacpp)
target_compile_definitions(idacpp_plugin PRIVATE
  IDACPP_VERSION="${PROJECT_VERSION}")

if(MSVC)
  target_compile_options(idacpp_plugin PRIVATE /EHsc)
elseif(UNIX)
  target_compile_options(idacpp_plugin PRIVATE -fPIC)
endif()

# Bake build-time paths as compile-time fallbacks for source-first mode.
# Normalize to forward slashes to avoid C escape sequence issues on Windows.
if(CLING_BUILD_DIR)
  file(TO_CMAKE_PATH "${CLING_BUILD_DIR}" _cling_build_dir)
  target_compile_definitions(idacpp_plugin PRIVATE
    CLINGLITE_DEFAULT_CLING_DIR="${_cling_build_dir}")
endif()
if(IDASDK)
  file(TO_CMAKE_PATH "${IDASDK}" _idasdk_path)
  target_compile_definitions(idacpp_plugin PRIVATE
    CLINGLITE_DEFAULT_IDASDK="${_idasdk_path}")
endif()

# ── Plugin system ─────────────────────────────────────────────────────────
# Discovers plugins, generates dispatch, assembles PCH, builds idgen table.
# Must come after idacpp_core and idacpp_plugin targets are defined.
add_subdirectory(plugins)
