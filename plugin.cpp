// plugin.cpp — IDA GUI plugin: C++ interpreter via Cling
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Registers a cli_t (command line interpreter tab) and an extlang_t
// so IDA's expression evaluator supports C++.

// IDA SDK headers
#include <pro.h>
#include <kernwin.hpp>
#include <expr.hpp>
#include <loader.hpp>
#include <idp.hpp>

// clinglite + extlang core
#include "extlang_core.h"
#include "idalib_setup.h"
#include "plugins/common/plugin_api.h"
#include <clinglite/clinglite.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#include "ida_sdk_identifiers.inc"

// Forward declarations
struct idacpp_t;
static idacpp_t* g_plugin = nullptr;

// ══════════════════════════════════════════════════════════════════════════════
// cli_t implementation
// ══════════════════════════════════════════════════════════════════════════════

static bool idaapi cli_execute_line(const char* line);
static bool idaapi cli_find_completions(
    qstrvec_t*, qstrvec_t*, qstrvec_t*, int*, int*, const char*, int);

static const cli_t cli_cpp = {
    sizeof(cli_t),
    0,                              // flags
    "c++",                          // sname (button text)
    "C++ interpreter",              // lname
    "Enter any C++ expression",     // hint
    cli_execute_line,
    nullptr,                        // keydown
    cli_find_completions,
};

// cli_execute_line and cli_find_completions defined below (after idacpp_t)

// ══════════════════════════════════════════════════════════════════════════════
// plugmod_t — plugin lifecycle
// ══════════════════════════════════════════════════════════════════════════════

namespace fs = std::filesystem;

struct idacpp_t : public plugmod_t {
    std::unique_ptr<clinglite::Environment> env;
    std::unique_ptr<clinglite::Interpreter> interp;
    bool extlang_installed = false;
    bool cli_installed = false;
    bool sdk_headers_ready = false;

    idacpp_t() = default;
    ~idacpp_t() override;

    bool idaapi run(size_t arg) override;
    bool init_plugin();
};

bool idacpp_t::init_plugin() {
    env = std::make_unique<clinglite::Environment>("idacpp");

    clinglite::Options opts;
    opts.args = {"idacpp"};

    // Resolve IDA SDK include directory
    std::string sdkInclude;
    qstring idasdk_buf;
    bool have_sdk = qgetenv("IDASDK", &idasdk_buf);
#ifdef CLINGLITE_DEFAULT_IDASDK
    if (!have_sdk) {
        // Use compile-time baked path, but verify it still exists
        if (fs::is_directory(CLINGLITE_DEFAULT_IDASDK))
            idasdk_buf = CLINGLITE_DEFAULT_IDASDK;
        else
            msg("idacpp: build-time IDASDK path no longer exists: %s\n"
                "  Set IDASDK environment variable to override.\n",
                CLINGLITE_DEFAULT_IDASDK);
        have_sdk = !idasdk_buf.empty();
    }
#endif
    if (have_sdk) {
        std::string base = idasdk_buf.c_str();
        if (fs::is_regular_file(fs::path(base) / "include" / "pro.h"))
            sdkInclude = (fs::path(base) / "include").string();
        else if (fs::is_regular_file(fs::path(base) / "src" / "include" / "pro.h"))
            sdkInclude = (fs::path(base) / "src" / "include").string();
        else
            msg("idacpp: IDASDK set to '%s' but pro.h not found there.\n",
                idasdk_buf.c_str());
    }
    if (!sdkInclude.empty()) {
        opts.includePaths.push_back(sdkInclude);
    }

    // Resolve Cling/LLVM build tree
    {
        qstring cling_dir_buf;
        if (qgetenv("CLING_DIR", &cling_dir_buf))
            opts.llvmDir = cling_dir_buf.c_str();
#ifdef CLINGLITE_DEFAULT_CLING_DIR
        else {
            if (fs::is_directory(CLINGLITE_DEFAULT_CLING_DIR))
                opts.llvmDir = CLINGLITE_DEFAULT_CLING_DIR;
            else
                msg("idacpp: build-time CLING_DIR no longer exists: %s\n"
                    "  Set CLING_DIR environment variable to override.\n",
                    CLINGLITE_DEFAULT_CLING_DIR);
        }
#endif
    }
    if (opts.llvmDir.empty()) {
        msg("idacpp: Cling/LLVM build tree not found.\n"
            "  Set CLING_DIR environment variable to the Cling build directory.\n");
        return false;
    }
    if (!fs::is_directory(opts.llvmDir)) {
        msg("idacpp: Cling/LLVM build tree not found at: %s\n"
            "  Set CLING_DIR environment variable to override.\n",
            opts.llvmDir.c_str());
        return false;
    }

    // Platform defines
    idacpp::prepareOptions(opts);

    // On-the-fly PCH generation from IDA SDK headers
    bool hasPch = false;
    if (!sdkInclude.empty())
        hasPch = idacpp::preparePCH(opts, sdkInclude);

    interp = std::make_unique<clinglite::Interpreter>(opts);
    if (!interp->isValid()) {
        msg("idacpp: failed to initialize C++ interpreter.\n"
            "  CLING_DIR: %s\n", opts.llvmDir.c_str());
        return false;
    }

    // Redirect output and diagnostics to IDA's message window
    interp->setOutputCallback([](const std::string& s) {
        msg("%s", s.c_str());
    });
    interp->setErrorCallback([](const std::string& s) {
        msg("%s", s.c_str());
    });

    // Load IDA libraries for JIT symbol resolution
    bool all_libs_loaded = true;
    {
#ifdef __NT__
        // MSVC CRT DLLs
        interp->loadLibrary("vcruntime140.dll");
        interp->loadLibrary("ucrtbase.dll");

        const char* idaLibs[] = {"ida.dll"};
#elif defined(__MAC__)
        const char* idaLibs[] = {"libida.dylib"};
#else
        const char* idaLibs[] = {"libida.so"};
#endif
        // Try loading from IDA installation directory first
        qstring idadir_buf;
        std::string idadir;
        if (qgetenv("IDADIR", &idadir_buf))
            idadir = idadir_buf.c_str();

        for (const char* lib : idaLibs) {
            bool loaded = false;
            if (!idadir.empty()) {
                std::string fullPath = (fs::path(idadir) / lib).string();
                loaded = (interp->loadLibrary(fullPath) ==
                          clinglite::ExecResult::Success);
            }
            if (!loaded)
                loaded = (interp->loadLibrary(lib) ==
                          clinglite::ExecResult::Success);
            if (!loaded) {
                all_libs_loaded = false;
                msg("idacpp: note: could not confirm runtime symbol load for %s\n",
                    lib);
            }
        }
    }

    if (hasPch) {
        sdk_headers_ready = true;
    } else {
        // No PCH — fall back to header-by-header inclusion
        if (!sdkInclude.empty())
            interp->addIncludePath(sdkInclude);
        if (sdkInclude.empty()) {
            msg("idacpp: IDA SDK headers unavailable — set IDASDK environment variable.\n"
                "  Plugin will load as a plain C++ REPL (no IDA API support).\n");
        } else if (!interp->includeHeader("pro.h")) {
            msg("idacpp: failed to include pro.h\n");
            return false;
        } else {
            int loaded_headers = 1; // pro.h
            int skipped_headers = 0;

            sdk_headers_ready = true;
            msg("idacpp: IDA SDK PCH unavailable, falling back to source headers\n");

            static const char* headers[] = {
                "ida.hpp", "idp.hpp", "kernwin.hpp", "bytes.hpp", "funcs.hpp",
                "segment.hpp", "name.hpp", "xref.hpp", "ua.hpp", "lines.hpp",
                "entry.hpp", "loader.hpp", "auto.hpp", "typeinf.hpp",
            };
            for (const char* hdr : headers) {
                if (interp->includeHeader(hdr))
                    ++loaded_headers;
                else
                    ++skipped_headers;
            }

            if (skipped_headers != 0) {
                msg("idacpp: warning: loaded %d IDA SDK headers, skipped %d\n",
                    loaded_headers, skipped_headers);
            } else {
                msg("idacpp: IDA SDK headers loaded from source (%d headers)\n",
                    loaded_headers);
            }
        }
    }

    // Run extension plugin setup
    clinglite::PluginSetupOptions pluginOpts;
    pluginOpts.hasPch = hasPch;
    clinglite::plugins::setupAll(*interp, pluginOpts);   // generic (linux, winsdk)
    idacpp::plugins::setupAll(*interp, pluginOpts);      // IDA-specific (qt6, idax)

    g_plugin = this;  // set before registering callbacks that use g_plugin

    // Register extlang
    idacpp::init(interp.get());
    idacpp::install();
    extlang_installed = true;

    // Register CLI tab
    install_command_interpreter(&cli_cpp);
    cli_installed = true;

    // Startup banner
    {
        auto clingVer = clinglite::Environment::version();

        // Build plugin list: ida_sdk (always) + clinglite + idacpp plugins
        std::string plugins = "ida_sdk";
        for (const auto& name : clinglite::plugins::pluginNames())
            plugins += ", " + name;
        for (const auto& name : idacpp::plugins::pluginNames())
            plugins += ", " + name;

        std::string banner = std::string("idacpp v") + IDACPP_VERSION
            + " (Cling " + clingVer;
        if (sdk_headers_ready)
            banner += " ; plugins: " + plugins;
        banner += ")";

        std::string sep(banner.size(), '-');
        msg("%s\n%s\n%s\n", sep.c_str(), banner.c_str(), sep.c_str());
    }
    return true;
}

idacpp_t::~idacpp_t() {
    if (cli_installed)
        remove_command_interpreter(&cli_cpp);

    if (extlang_installed)
        idacpp::remove();

    idacpp::term();

    if (g_plugin == this)
        g_plugin = nullptr;
}

bool idaapi idacpp_t::run(size_t /*arg*/) {
    return true;
}

// cli_execute_line (needs complete idacpp_t)
static bool idaapi cli_execute_line(const char* line) {
    auto* session = idacpp::getSession();
    if (!session)
        return true;

    int crashCode = 0;
    clinglite::ExecResult res;
    int indent = session->processLine(line, &res, &crashCode);

    if (crashCode != 0) {
        msg("C++ crash caught (code 0x%08x), recovering...\n", crashCode);
        return true;
    }

    return indent <= 0;
}

// cli_find_completions (needs complete idacpp_t)
static const char* const s_dotCommands[] = {
    ".L", ".x", ".X", ".U", ".I", ".include",
    ".O", ".class", ".Class", ".namespace", ".typedef",
    ".files", ".fileEx", ".g",
    ".@", ".rawInput", ".dynamicExtensions",
    ".debug", ".printDebug",
    ".storeState", ".compareState", ".stats",
    ".T", ".trace",
    ".undo", ".help", ".?", ".q",
    ".clear",   // clinglite Session extension
    ".>",       // redirect
};

static bool idaapi cli_find_completions(
    qstrvec_t* out_completions,
    qstrvec_t* out_hints,
    qstrvec_t* out_docs,
    int* out_match_start,
    int* out_match_end,
    const char* line,
    int x)
{
    if (!g_plugin || !g_plugin->interp)
        return false;

    std::string input(line);
    size_t cursor = static_cast<size_t>(x);
    if (cursor > input.size())
        cursor = input.size();

    // Check if line starts with a dot command
    std::string upToCursor = input.substr(0, cursor);
    size_t tokenStart = upToCursor.find_first_not_of(" \t");
    bool isDotCommand = (tokenStart != std::string::npos
                         && upToCursor[tokenStart] == '.');

    if (isDotCommand) {
        std::string typed = upToCursor.substr(tokenStart);
        // Only complete the command name (not arguments after a space)
        if (typed.find(' ') != std::string::npos)
            return false;

        *out_match_start = static_cast<int>(tokenStart);
        *out_match_end   = static_cast<int>(cursor);

        for (const char* cmd : s_dotCommands) {
            if (strncmp(cmd, typed.c_str(), typed.size()) == 0)
                out_completions->push_back(cmd);
        }
        return !out_completions->empty();
    }

    // Compute prefix by walking backwards from cursor through identifier chars
    size_t start = cursor;
    while (start > 0) {
        char c = input[start - 1];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            --start;
        else
            break;
    }

    std::string prefix = input.substr(start, cursor - start);
    if (prefix.empty())
        return false;

    *out_match_start = static_cast<int>(start);
    *out_match_end   = static_cast<int>(cursor);

    // Static table lookup — O(log n) prefix search, instant
    auto cmpPrefix = [](const char* entry, const std::string& pfx) {
        return strncmp(entry, pfx.c_str(), pfx.size()) < 0;
    };
    auto* begin = g_idaSdkIdentifiers;
    auto* end = begin + g_idaSdkIdentifierCount;
    auto it = std::lower_bound(begin, end, prefix, cmpPrefix);
    while (it != end && strncmp(*it, prefix.c_str(), prefix.size()) == 0)
        out_completions->push_back(*it++);

    return !out_completions->empty();
}

// ══════════════════════════════════════════════════════════════════════════════
// plugin_t entry point
// ══════════════════════════════════════════════════════════════════════════════

static plugmod_t* idaapi idacpp_init() {
    auto* p = new idacpp_t();
    if (p->init_plugin())
        return p;
    delete p;
    return nullptr;
}

plugin_t PLUGIN = {
    IDP_INTERFACE_VERSION,
    PLUGIN_FIX | PLUGIN_HIDE | PLUGIN_MULTI,
    idacpp_init,            // init
    nullptr,                    // term (nullptr for PLUGIN_MULTI)
    nullptr,                    // run  (nullptr for PLUGIN_MULTI)
    "C++ interpreter for IDA",          // comment
    "C++ REPL and expression evaluator\n",  // help
    "idacpp",                 // wanted_name
    nullptr,                    // wanted_hotkey
};
