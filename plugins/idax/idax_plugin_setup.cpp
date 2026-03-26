// idax_plugin_setup.cpp — idax extension plugin setup.
//
// Called by the generated plugin dispatch after IDA SDK initialization.
// Uses compile-time baked paths (IDACPP_PLUGIN_IDAX_INCLUDE_DIR,
// IDACPP_PLUGIN_IDAX_SHARED_LIB) set by the plugin's CMakeLists.txt.

#include "idax_plugin_setup.h"
#include <clinglite/clinglite.h>
#include <iostream>

#ifndef IDACPP_PLUGIN_IDAX_INCLUDE_DIR
#error "IDACPP_PLUGIN_IDAX_INCLUDE_DIR not defined — check plugin CMakeLists.txt"
#endif
#ifndef IDACPP_PLUGIN_IDAX_SHARED_LIB
#error "IDACPP_PLUGIN_IDAX_SHARED_LIB not defined — check plugin CMakeLists.txt"
#endif

void idax_plugin_setup(clinglite::Interpreter& interp, clinglite::PluginSetupOptions& /*opts*/) {
    // Load idax shared library for JIT symbol resolution.
    // Path baked at compile time by the plugin's CMakeLists.txt.
    const char* sharedLib = IDACPP_PLUGIN_IDAX_SHARED_LIB;
    auto r = interp.loadLibrary(sharedLib);
    if (r == clinglite::ExecResult::Success)
        std::cerr << "Plugin idax: loaded shared library\n";
    else
        std::cerr << "Plugin idax: warning — failed to load " << sharedLib << "\n";

    // Add idax include path for header resolution
    const char* includeDir = IDACPP_PLUGIN_IDAX_INCLUDE_DIR;
    interp.addIncludePath(includeDir);

    // Undo pro.h macro poisoning (no-op if already undefined via PCH).
    clinglite::undoProhPoisoning(interp);

    // Include idax master header — hits include guards if already in PCH.
    if (!interp.includeHeader("ida/idax.hpp"))
        std::cerr << "Plugin idax: warning — failed to include ida/idax.hpp\n";
    else
        std::cerr << "Plugin idax: ready\n";
}
