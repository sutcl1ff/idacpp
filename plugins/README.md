# idacpp Plugin System

Plugins extend the idacpp REPL with additional headers, libraries, and PCH
contributions. Each plugin lives in its own subdirectory and is self-contained.

## Directory Layout

| Directory | Type | Description |
|-----------|------|-------------|
| `common/` | Framework | Shared cmake helpers and public API header |
| `ida_sdk/` | Built-in | IDA SDK header list — always enabled, loads first |
| `idax/` | Extension | C++23 IDA SDK wrapper (`-DPLUGIN_IDAX_SRC_DIR=<path>`) |
| `winsdk/` | Extension | Windows SDK headers |
| `template/` | Skeleton | Copy this to create a new plugin |

## Creating a New Plugin

1. Copy `template/` to `plugins/<your-name>/`
2. Rename placeholders (`<PLUGIN_NAME>`, `<plugin_name>`)
3. Implement `CMakeLists.txt` using the shared helpers
4. Implement the setup function in `<name>_plugin_setup.cpp`
5. Enable with `-DPLUGIN_<YOUR_NAME>_SRC_DIR=<path>`

See [`template/README.md`](template/README.md) for a detailed walkthrough.

## How It Works

1. **Discovery**: `plugins/CMakeLists.txt` scans for `*/CMakeLists.txt` and
   includes each directory whose `PLUGIN_<NAME>` variable is `ON`. Plugins
   with a `_SRC_DIR` variable are auto-enabled when the source dir is provided.
2. **Registration**: Each plugin calls `idacpp_register_plugin()` to register
   a setup function. The framework collects these into a generated
   `plugin_dispatch.cpp` with a single `setupAll()` entry point.
3. **PCH assembly**: Plugins call `idacpp_plugin_extend_pch()` to contribute
   headers and include flags. The framework assembles one PCH from all
   contributions (IDA SDK base + extension plugins).
4. **Runtime setup**: After interpreter initialization, `setupAll()` calls
   each plugin's setup function with the interpreter instance. Plugins use
   baked compile-time paths to load shared libraries and include headers.

## Shared Helpers (`common/idacpp_plugin_helpers.cmake`)

| Function | Purpose |
|----------|---------|
| `idacpp_register_plugin(name fn header source)` | Register setup function with dispatch |
| `idacpp_plugin_extend_pch(FLAGS ... HEADERS ...)` | Add headers/flags to shared PCH |
| `idacpp_plugin_generate_pch_bridge(file UNDEFS ... HEADERS ...)` | Generate `#undef` + `#include` bridge header |
| `idacpp_plugin_make_shared(target static_lib LINK_LIBS ...)` | Wrap static archive as shared lib for JIT |

## Plugin Load Order

1. **ida_sdk** (built-in) — IDA SDK headers and libraries via `idalib_setup.cpp`
2. **Extension plugins** — in directory scan order, via generated `setupAll()`

## Conventions

- Plugin cmake variables use the `PLUGIN_<NAME>_` prefix (e.g. `PLUGIN_IDAX_SRC_DIR`)
- Setup functions are named `<name>_plugin_setup(clinglite::Interpreter&, bool hasPch)`
- All setup operations should be idempotent (safe to call with or without PCH)
- Plugins that layer on top of `pro.h` should use `idacpp_plugin_generate_pch_bridge()`
  to `#undef` poisoned macros before including their headers
