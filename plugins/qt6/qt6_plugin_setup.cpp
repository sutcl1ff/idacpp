// qt6_plugin_setup.cpp — Qt6 extension plugin setup.
//
// Called by the generated plugin dispatch after IDA SDK initialization.
// Uses compile-time baked paths (IDACPP_PLUGIN_QT6_*) set by the plugin's
// CMakeLists.txt.

#include "qt6_plugin_setup.h"
#include <clinglite/clinglite.h>
#include <iostream>

#ifndef IDACPP_PLUGIN_QT6_INCLUDE_DIR
#error "IDACPP_PLUGIN_QT6_INCLUDE_DIR not defined — check plugin CMakeLists.txt"
#endif
#ifndef IDACPP_PLUGIN_QT6_CORE_LIB
#error "IDACPP_PLUGIN_QT6_CORE_LIB not defined — check plugin CMakeLists.txt"
#endif
#ifndef IDACPP_PLUGIN_QT6_GUI_LIB
#error "IDACPP_PLUGIN_QT6_GUI_LIB not defined — check plugin CMakeLists.txt"
#endif
#ifndef IDACPP_PLUGIN_QT6_WIDGETS_LIB
#error "IDACPP_PLUGIN_QT6_WIDGETS_LIB not defined — check plugin CMakeLists.txt"
#endif

void qt6_plugin_setup(clinglite::Interpreter& interp, bool /*hasPch*/) {
    // Load Qt6 shared libraries for JIT symbol resolution.
    // IDA has already loaded these DLLs; this tells Cling's JIT where to
    // find Qt symbols. Paths baked at compile time by the plugin's CMakeLists.txt.
    struct QtLib { const char* name; const char* path; };
    QtLib libs[] = {
        {"Qt6Core",    IDACPP_PLUGIN_QT6_CORE_LIB},
        {"Qt6Gui",     IDACPP_PLUGIN_QT6_GUI_LIB},
        {"Qt6Widgets", IDACPP_PLUGIN_QT6_WIDGETS_LIB},
    };

    int loaded = 0;
    for (const auto& lib : libs) {
        auto r = interp.loadLibrary(lib.path);
        if (r == clinglite::ExecResult::Success)
            ++loaded;
        else
            std::cerr << "Plugin qt6: warning — failed to load " << lib.name
                      << " (" << lib.path << ")\n";
    }

    // Add Qt6 include path for header resolution
    interp.addIncludePath(IDACPP_PLUGIN_QT6_INCLUDE_DIR);

    // Undo pro.h macro poisoning (snprintf→dont_use_snprintf, etc.)
    // These are no-ops if the macros are already undefined (PCH case).
    interp.execute("#undef snprintf");
    interp.execute("#undef sprintf");
    interp.execute("#undef getenv");

    // IDA's Qt is always built with QT_NAMESPACE=QT.
    // Define it so all Qt headers put classes in the QT namespace.
    interp.execute("#define QT_NAMESPACE QT");
    interp.execute("#define QT_CORE_LIB");
    interp.execute("#define QT_GUI_LIB");
    interp.execute("#define QT_WIDGETS_LIB");
    interp.execute("#define QT_DLL");

    // Include Qt6 umbrella headers — hits include guards if already in PCH.
    static const char* headers[] = {
        "QtCore/QtCore",
        "QtGui/QtGui",
        "QtWidgets/QtWidgets",
    };

    int hdrs = 0;
    for (const char* hdr : headers) {
        if (interp.includeHeader(hdr))
            ++hdrs;
        else
            std::cerr << "Plugin qt6: warning — failed to include " << hdr << "\n";
    }

    // Convenience: bring QT namespace into scope so users can write QWidget*
    // instead of QT::QWidget*.
    interp.execute("using namespace QT;");

    if (loaded == 3 && hdrs == 3)
        std::cerr << "Plugin qt6: ready\n";
    else
        std::cerr << "Plugin qt6: " << loaded << "/3 libraries, "
                  << hdrs << "/3 headers\n";
}
