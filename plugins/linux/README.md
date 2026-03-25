# linux — Extension Plugin

Preloads POSIX/Linux system headers into the idacpp REPL so users can
reference ELF structures and POSIX APIs when analyzing Linux binaries.

## Headers included

`elf.h`, `link.h`, `dlfcn.h`, `unistd.h`, `fcntl.h`, `signal.h`,
`sys/mman.h`, `sys/stat.h`, `sys/types.h`, `sys/wait.h`, `dirent.h`

## Enable

```bash
cmake .. -DPLUGIN_LINUX=ON
```

Only builds on Linux. Silently skipped on other platforms.
