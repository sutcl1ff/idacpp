# Building idacpp

## Prerequisites

- CMake 3.20+
- C++17 compiler (MSVC, GCC, or Clang)
- IDA SDK (9.x)
- A [clinglite](https://github.com/0xeb/clinglite) source checkout
- A pre-built Cling/LLVM tree — see clinglite's [BUILDING.md](https://github.com/0xeb/clinglite/blob/main/BUILDING.md) for build instructions
- Runtime access to the Cling build tree (baked in at compile time or via `CLING_DIR`)

## Runtime model

idacpp runs in **source-first** mode: the LLVM/Cling compiler infrastructure is statically linked into the plugin, but runtime resources (Clang built-in headers, Cling runtime headers, C++ standard library headers) are resolved from a Cling build tree at runtime.

## CMake variables

| Variable | Description |
|----------|-------------|
| `CLINGLITE_SOURCE_DIR` | Path to your clinglite source checkout |
| `CLING_BUILD_DIR` | Path to a Cling/LLVM build tree |
| `IDASDK` | Path to the IDA SDK root (bootstrap auto-detects GitHub-clone layouts) |

Each variable can be set via `-D` flag or environment variable.

## Windows (MSVC, fully static)

Requires LLVM built with `/MT` (static CRT). The resulting `idacpp.dll` has zero MSVC runtime dependencies.

```bash
mkdir build && cd build
cmake -A x64 \
    -DCLINGLITE_SOURCE_DIR=C:/path/to/clinglite \
    -DCLING_BUILD_DIR=C:/path/to/cling-build \
    -DIDASDK=C:/path/to/idasdk \
    ..
cmake --build . --config Release -j 8
```

On Windows, LLVM must be built with `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` (static CRT) to produce a self-contained plugin DLL. If you get **LNK2038 CRT mismatch** errors, both LLVM and idacpp must use the same MSVC runtime — build both with `/MT`.

## Linux / macOS

```bash
mkdir build && cd build
cmake -G Ninja \
    -DCLINGLITE_SOURCE_DIR=/path/to/clinglite \
    -DCLING_BUILD_DIR=/path/to/cling-build \
    -DIDASDK=/path/to/idasdk \
    -DCMAKE_BUILD_TYPE=Release \
    ..
cmake --build . -j$(nproc)  # macOS: use -j$(sysctl -n hw.ncpu)
```

## Build output

The build produces `idacpp.dll` / `idacpp.so` / `idacpp.dylib` (placed in the IDA SDK plugin directory by `ida_add_plugin()`).

| Platform | Output | Size | Runtime dependencies |
|----------|--------|------|---------------------|
| Windows MSVC `/MT` | `idacpp.dll` | ~114 MB | `ida.dll` + Windows system DLLs only |
| Linux GCC | `idacpp.so` | ~152 MB | `libida.so` + system libs |
| macOS Clang | `idacpp.dylib` | ~152 MB | `libida.dylib` + system libs |

## IDA SDK include order

**Always include standard C/C++ headers BEFORE `<pro.h>`.**

`pro.h` poisons `getenv`, `fprintf`, `snprintf`, `stderr` with macros. If standard headers come after, the binary imports symbols from the SDK link stubs whose global constructors abort headless binaries.

```cpp
// CORRECT — standard headers first
#include <cstdio>
#include <string>

#include <clinglite/clinglite.h>
#include <extlang_core.h>

#include <pro.h>     // IDA SDK — LAST
#include <expr.hpp>
```

## Project structure

| File | Description |
|------|-------------|
| `plugin.cpp` | IDA plugin entry point (`idacpp.dll` / `idacpp.so` / `idacpp.dylib`) |
| `extlang_core.cpp/h` | C++ extlang implementation (compile, evaluate, completions) |
| `cpp_highlighter.cpp/h` | C++ syntax highlighting for the CLI tab |
| `cpp_highlight_keywords.h` | C++ and IDA SDK keyword lists for syntax highlighting |
| `idalib_setup.cpp/h` | Headless IDA setup for standalone consumers (idalib) |
| `ida_sdk.h` | Curated IDA SDK umbrella header for PCH generation |
| `tools/idgen.cpp` | Build-time tool: generates sorted IDA SDK identifier table for completion and highlighting |
| `tools/sync_ida_sdk_headers.py` | Utility: synchronizes `ida_sdk.h` include list with the IDA SDK headers on disk |
