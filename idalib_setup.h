// idalib_setup.h — headless IDA setup API for standalone consumers
//
// No IDA types leak through this header. Consumers only need clinglite.

#pragma once

#include <clinglite/clinglite.h>
#include <string>

namespace idacpp {

struct IdaSession {
    bool headersReady = false;
    bool libInitialized = false;
    bool dbOpen = false;
};

/// Generate or load a cached IDA SDK PCH.
/// Caches to cacheDir (default: get_user_idadir()/idacpp/). If a cached
/// PCH matching the current configuration (SDK path, compiler flags, header
/// mtimes) exists, sets opts.pchPath and returns true. Otherwise, generates
/// one on-the-fly using a temporary interpreter.
/// sdkInclude: path to IDA SDK include directory (e.g. IDASDK/include).
/// cacheDir: override cache directory (empty = default get_user_idadir()/idacpp/).
/// Returns true if PCH is available (opts.pchPath set).
bool preparePCH(clinglite::Options& opts,
                const std::string& sdkInclude,
                const std::string& cacheDir = "");

/// Resolve Cling/LLVM build directory.
/// Checks CLING_DIR env, then compile-time CLINGLITE_DEFAULT_CLING_DIR
/// (only defined in the plugin build, not in idacpp_core consumers).
std::string resolveClingDir();

/// Resolve IDA runtime directory from IDADIR env, then IDASDK env (/bin),
/// then compile-time CLINGLITE_DEFAULT_IDASDK (/bin).
/// Compile-time defaults are only defined in the plugin build.
/// Returns empty string if nothing found.
std::string resolveIdaDir();

/// Resolve IDA SDK include directory from IDASDK env or compile-time default.
/// Probes both $IDASDK/include and $IDASDK/src/include layouts.
/// Compile-time defaults are only defined in the plugin build.
/// Returns empty string if nothing found.
std::string resolveSdkInclude();

/// Full headless IDA setup: load libraries, init_library(), include headers,
/// register extlang. Call AFTER Interpreter is created and valid.
IdaSession initIda(clinglite::Interpreter& interp,
                   const std::string& idaDir,
                   const std::string& dbPath,
                   bool hasPch);

/// Cleanup: unregister extlang, close database.
void termIda(clinglite::Interpreter& interp, const IdaSession& session);

} // namespace idacpp
