// <plugin_name>_plugin_setup.cpp — <PLUGIN_NAME> extension plugin setup.
//
// Rename this file to <plugin_name>_plugin_setup.cpp
// Called by the generated plugin dispatch after IDA SDK initialization.

#include "template_plugin_setup.h"
#include <clinglite/clinglite.h>
#include <iostream>

void template_plugin_setup(clinglite::Interpreter& interp, bool /*hasPch*/) {
    // Example: load a shared library for JIT symbol resolution
    // interp.loadLibrary(PLUGIN_<PLUGIN_NAME>_SHARED_LIB);

    // Example: add include path
    // interp.addIncludePath(PLUGIN_<PLUGIN_NAME>_INCLUDE_DIR);

    // Example: undo pro.h macro poisoning if your headers need std functions
    // interp.execute("#undef snprintf");
    // interp.execute("#undef sprintf");
    // interp.execute("#undef getenv");

    // Example: include your master header
    // interp.includeHeader("<your/master_header.hpp>");

    std::cerr << "Plugin <plugin_name>: ready\n";
}
