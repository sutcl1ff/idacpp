// idax_plugin_setup.h — idax extension plugin for idacpp.
#pragma once

namespace clinglite { class Interpreter; struct PluginSetupOptions; }

/// Set up the idax plugin on an initialized interpreter.
/// Loads the idax shared library, adds include paths, undoes pro.h macro
/// poisoning, and includes the idax master header.
void idax_plugin_setup(clinglite::Interpreter& interp, clinglite::PluginSetupOptions& opts);
