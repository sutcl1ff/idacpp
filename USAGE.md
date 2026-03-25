# Using idacpp

idacpp exposes the C++ interpreter through two IDA interfaces: a **CLI tab** for interactive use and an **expression evaluator** for programmatic evaluation. Both share the same interpreter session — definitions made in one are visible to the other.

## CLI commands

Select the **C++** tab in IDA's output window to get an interactive REPL. The CLI supports Cling's dot commands:

| Command | Description |
|---------|-------------|
| `.help` (or `.?`) | Show all available commands |
| `.L <file>` | Load and execute a file or library |
| `.x <file>[(args)]` | Load file and call its entry function |
| `.U <file>` | Unload a previously loaded file |
| `.I [path]` | Show or add include paths |
| `.O [level]` | Show or set optimization level (0-3) |
| `.class <name>` | Print class layout (one level) |
| `.Class <name>` | Print class layout (all levels) |
| `.namespace` | List all known namespaces |
| `.g [var]` | Print global variable info |
| `.undo [n]` | Undo the last N inputs |
| `.clear` | Undo all user inputs, restoring initial state |
| `.@` | Cancel multiline input |

## Script format

IDA C++ scripts should put all imperative code inside functions with a conventional entrypoint:

```cpp
#include <funcs.hpp>
#include <segment.hpp>

int main() {
    int n = get_func_qty();
    msg("functions: %d\n", n);
    return 0;
}
```

**Rules:**
1. Entrypoint: `int main()` or `int main(int argc, char** argv)`
2. File scope may contain: `#include` directives, function/class/struct definitions, global variable declarations, `using` directives
3. **No bare expression statements at file scope** (e.g., `msg(...)`, `vec.push_back(...)`) — these must go inside functions

Scripts are loaded and executed via `.x` (CLI) or Alt-F7 (IDA UI). The entrypoint is invoked automatically. Loading the same file again auto-unloads the previous version.

## Expression evaluator

When C++ is the active language, IDA's expression evaluator uses idacpp. The evaluator handles several operations, each with different scoping:

- **Evaluate expression** — executes at global scope, returns a value
- **Execute snippet** — multi-line snippets are wrapped in `{ }` so local variables don't leak into the global session; single-line snippets run at global scope
- **Compile file** — loads file via Cling's `.L` command; auto-unloads previous version of the same file. If a namespace is provided, wraps in `namespace name { ... }` instead
- **Compile expression** — wraps the expression in a named function at global scope for later invocation

## Runtime setup

The plugin can use compile-time fallback paths baked in at build time, but on a different machine you will usually want to set the runtime paths explicitly:

```bash
export CLING_DIR=/path/to/cling-build
export IDASDK=/path/to/idasdk
export IDADIR=/path/to/ida-pro   # optional
```

| Variable | Required | Effect when missing |
|----------|----------|-------------------|
| `CLING_DIR` | Yes | Plugin cannot initialize Cling and will not start |
| `IDASDK` | No | Plugin still loads as a plain C++ REPL (no IDA SDK types) |
| `IDADIR` | No | Runtime library lookup uses default loader search paths |

When `IDASDK` is present, the plugin preloads IDA SDK headers through a cached PCH for instant startup.
