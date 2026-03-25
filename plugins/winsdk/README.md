# winsdk — Extension Plugin

Preloads Windows SDK headers into the idacpp REPL so users can reference
Win32 types and APIs when analyzing Windows binaries.

## Headers included

`windows.h`, `winsock2.h`, `ws2tcpip.h`, `shlwapi.h`, `dbghelp.h`,
`tlhelp32.h`, `psapi.h`, `winternl.h`

## Enable

```bash
cmake .. -DPLUGIN_WINSDK=ON
```

Only builds on Windows (MSVC/MinGW). Silently skipped on other platforms.
