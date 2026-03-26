// qt6_plugin_setup.h — Qt6 extension plugin for idacpp.
#pragma once

namespace clinglite { class Interpreter; struct PluginSetupOptions; }

/// Set up the Qt6 plugin on an initialized interpreter.
/// Loads Qt6 shared libraries, adds include paths, defines QT_NAMESPACE=QT,
/// and includes Qt6 umbrella headers (Core, Gui, Widgets).
void qt6_plugin_setup(clinglite::Interpreter& interp, clinglite::PluginSetupOptions& opts);
