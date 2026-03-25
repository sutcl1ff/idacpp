# ida_sdk — Built-in Plugin

The `ida_sdk` plugin is the base layer of the idacpp plugin system. It is
**always enabled** and loads first, before any extension plugins.

## What it provides

- **`ida_sdk.h`** — curated list of 56+ IDA SDK headers used for:
  - Build-time PCH generation (`clinglite_pchgen`)
  - Identifier table generation (`idacpp_idgen`)
  - Runtime PCH generation (`preparePCH()` in `idalib_setup.cpp`)

## Runtime setup

Runtime initialization (library loading, PCH caching, extlang registration)
is handled by the existing `idalib_setup.cpp`. This plugin only provides the
build-time header configuration.

## Modifying the header list

Edit `ida_sdk.h` to add or remove IDA SDK headers. The build system
automatically re-parses the file on the next configure. Keep the list
ordered and conservative — broad enough for useful SDK coverage, but not
every public SDK header belongs here.
