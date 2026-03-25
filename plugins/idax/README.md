# idax — Extension Plugin

The `idax` plugin integrates [idax](https://github.com/19h/idax) — a C++23
opaque wrapper over the IDA Pro SDK by
[Kenan Sulayman](https://github.com/19h) — into the idacpp REPL. When
enabled, users can immediately use the idax API without any manual setup:

```cpp
// These just work — no #include or #undef needed
for (auto fn : ida::function::all())
    std::cout << fn.name() << "\n";

auto count = ida::segment::count().value_or(0);
```

## Enable

```bash
cmake .. -DPLUGIN_IDAX_SRC_DIR=/path/to/idax
```

## What it does

1. **Builds** idax as a static + shared library from `PLUGIN_IDAX_SRC_DIR`
2. **Generates** a PCH bridge header (`#undef` pro.h poisons + `#include <ida/idax.hpp>`)
3. **Extends** the build-time PCH with idax headers
4. **At runtime**: loads the shared library, adds include paths, includes headers

## Requirements

- `PLUGIN_IDAX_SRC_DIR` must point to an idax source tree with `include/ida/idax.hpp`
- IDA SDK must be available (idax depends on `idasdk_headers` and `idasdk::idalib`)

## Attribution

**idax** — *The IDA SDK, redesigned for humans.*
- Author: [Kenan Sulayman](https://github.com/19h) (<kenan@int.mov>)
- Repository: [github.com/19h/idax](https://github.com/19h/idax)
- License: MIT
