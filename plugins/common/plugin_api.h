// plugin_api.h — idacpp plugin system public API.
//
// Call setupAll() after interpreter + IDA SDK initialization to run
// all registered IDA-specific plugin setup functions (qt6, idax, etc.).

#pragma once

#include <clinglite/clinglite.h>
#include <string>
#include <vector>

namespace idacpp { namespace plugins {

/// Run setup for all registered IDA-specific extension plugins.
/// Call after initIda() completes. Each plugin loads its shared library,
/// adds include paths, and includes headers via the interpreter.
void setupAll(clinglite::Interpreter& interp, clinglite::PluginSetupOptions& opts);

/// Return names of all enabled IDA-specific extension plugins (baked at build time).
std::vector<std::string> pluginNames();

}} // namespace idacpp::plugins
