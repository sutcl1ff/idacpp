// linux_plugin_setup.h — Linux/POSIX extension plugin for idacpp.
#pragma once

namespace clinglite { class Interpreter; }

/// Preload POSIX/Linux system headers into the interpreter.
void linux_plugin_setup(clinglite::Interpreter& interp, bool hasPch);
