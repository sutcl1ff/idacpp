// plugin_api.h — idacpp plugin system public API.
//
// Call setupAll() after interpreter + IDA SDK initialization to run
// all registered extension plugin setup functions (idax, winsdk, etc.).

#pragma once

#include <string>
#include <vector>

namespace clinglite { class Interpreter; }

namespace idacpp { namespace plugins {

/// Run setup for all registered extension plugins.
/// Call after initIda() completes. Each plugin loads its shared library,
/// adds include paths, and includes headers via the interpreter.
void setupAll(clinglite::Interpreter& interp, bool hasPch);

/// Return names of all enabled extension plugins (baked at build time).
std::vector<std::string> pluginNames();

}} // namespace idacpp::plugins
