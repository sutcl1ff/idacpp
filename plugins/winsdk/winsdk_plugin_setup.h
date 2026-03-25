// winsdk_plugin_setup.h — Windows SDK extension plugin for idacpp.
#pragma once

namespace clinglite { class Interpreter; }

/// Preload Windows SDK headers into the interpreter.
void winsdk_plugin_setup(clinglite::Interpreter& interp, bool hasPch);
