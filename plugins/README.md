# idacpp Plugin System

IDA-specific plugins that extend the idacpp REPL. Built on top of clinglite's
generic plugin infrastructure (`clinglite/plugins/common/clinglite_plugin_helpers.cmake`).

Generic platform plugins (linux, winsdk) live in clinglite. This directory
contains only IDA-specific plugins.

## Directory Layout

| Directory | Type | Description |
|-----------|------|-------------|
| `common/` | Framework | Thin wrappers around clinglite helpers + public API header |
| `ida_sdk/` | Built-in | IDA SDK header list — always enabled, loads first |
| `idax/` | Extension | C++23 IDA SDK wrapper (`-DPLUGIN_IDAX_SRC_DIR=<path>`) |
| `qt6/` | Extension | Qt6 support — Core, Gui, Widgets (`-DPLUGIN_QT6=ON`) |

## Creating a New Plugin

See `clinglite/plugins/template/` for a detailed walkthrough.

1. Copy the template to `plugins/<your-name>/`
2. Rename placeholders (`<PLUGIN_NAME>`, `<plugin_name>`)
3. Use `idacpp_register_plugin()` / `idacpp_plugin_extend_pch()` for IDA dispatch
4. Use `clinglite_plugin_generate_pch_bridge()` / `clinglite_plugin_make_shared()` directly
5. Enable with `-DPLUGIN_<YOUR_NAME>_SRC_DIR=<path>`

## How It Works

1. **Discovery**: `plugins/CMakeLists.txt` scans for `*/CMakeLists.txt` and
   includes each directory whose `PLUGIN_<NAME>` variable is `ON`.
2. **Registration**: Each plugin calls `idacpp_register_plugin()` (thin wrapper
   around clinglite's `_clinglite_plugin_register_impl(IDACPP, ...)`).
3. **Dispatch**: `clinglite_generate_plugin_dispatch(IDACPP ...)` generates
   `plugin_dispatch.cpp` with `idacpp::plugins::setupAll()`.
4. **PCH assembly**: Merges clinglite plugin + idacpp plugin PCH contributions.
5. **Runtime**: `clinglite::plugins::setupAll()` runs first (generic plugins),
   then `idacpp::plugins::setupAll()` (IDA-specific plugins).

## Helpers

| Function | Source | Purpose |
|----------|--------|---------|
| `idacpp_register_plugin()` | idacpp wrapper | Register with IDACPP dispatch |
| `idacpp_plugin_extend_pch()` | idacpp wrapper | Add to IDACPP PCH contributions |
| `clinglite_plugin_generate_pch_bridge()` | clinglite | Generate bridge header |
| `clinglite_plugin_make_shared()` | clinglite | Wrap static→shared for JIT |
| `clinglite_generate_plugin_dispatch()` | clinglite | Generate dispatch .cpp |

## Plugin Load Order

1. **clinglite plugins** — generic (linux, winsdk) via `clinglite::plugins::setupAll()`
2. **ida_sdk** (built-in) — IDA SDK headers and libraries via `idalib_setup.cpp`
3. **idacpp plugins** — IDA-specific (qt6, idax) via `idacpp::plugins::setupAll()`

## Conventions

- Plugin cmake variables use the `PLUGIN_<NAME>_` prefix
- Setup functions: `<name>_plugin_setup(clinglite::Interpreter&, clinglite::PluginSetupOptions&)`
- All setup operations should be idempotent (safe to call with or without PCH)
- Plugins that layer on top of `pro.h` use `clinglite_plugin_generate_pch_bridge()`
