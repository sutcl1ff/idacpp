// idalib_setup.cpp — headless IDA setup implementation
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Provides preparePCH() / initIda() / termIda() for standalone consumers
// that want to set up IDA libraries, headers, and extlang without being a plugin.

#include "idalib_setup.h"
#include "extlang_core.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#include <pro.h>
#include <diskio.hpp>
#include <idalib.hpp>
#include <expr.hpp>

#include <chrono>
#include <climits>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace idacpp {

// ── C++ stdlib headers to include in PCH ─────────────────────────────────────

static const char* g_stdHeaders[] = {
    "cstdio", "cstdlib", "cstring", "cstdint", "cmath", "cassert",
    "string", "vector", "map", "set", "unordered_map", "unordered_set",
    "algorithm", "functional", "memory", "utility", "iostream",
    "sstream", "fstream", "array", "deque", "list", "optional",
    "variant", "tuple", "numeric", "iterator", "type_traits",
};

// ── IDA SDK header list for PCH generation ───────────────────────────────────

static const char* g_idaHeaders[] = {
    "pro.h",
    "auto.hpp",
    "allins.hpp",
    "bitrange.hpp",
    "bytes.hpp",
    "compress.hpp",
    "config.hpp",
    "cvt64.hpp",
    "dbg.hpp",
    "demangle.hpp",
    "diff3.hpp",
    "dirtree.hpp",
    "diskio.hpp",
    "entry.hpp",
    "err.h",
    "exehdr.h",
    "expr.hpp",
    "fixup.hpp",
    "fpro.h",
    "frame.hpp",
    "funcs.hpp",
    "gdl.hpp",
    "graph.hpp",
    "help.h",
    "hexrays.hpp",
    "ida.hpp",
    "ida_highlighter.hpp",
    "idacfg.hpp",
    "idalib.hpp",
    "idd.hpp",
    "idp.hpp",
    "ieee.h",
    "jumptable.hpp",
    "kernwin.hpp",
    "lex.hpp",
    "libfuncs.hpp",
    "lines.hpp",
    "llong.hpp",
    "loader.hpp",
    "lumina.hpp",
    "make_script_ns.hpp",
    "md5.h",
    "merge.hpp",
    "mergemod.hpp",
    "moves.hpp",
    "nalt.hpp",
    "name.hpp",
    "netnode.hpp",
    "network.hpp",
    "offset.hpp",
    "parsejson.hpp",
    "problems.hpp",
    "prodir.h",
    "pronet.h",
    "range.hpp",
    "regfinder.hpp",
    "registry.hpp",
    "search.hpp",
    "segment.hpp",
    "segregs.hpp",
    "srclang.hpp",
    "strlist.hpp",
    "tryblks.hpp",
    "typeinf.hpp",
    "ua.hpp",
    "undo.hpp",
    "workarounds.hpp",
    "xref.hpp"
};

// ── Public path resolution helpers ───────────────────────────────────────────

std::string resolveClingDir() {
    qstring env;
    if (qgetenv("CLING_DIR", &env) && !env.empty())
        return env.c_str();
#ifdef CLINGLITE_DEFAULT_CLING_DIR
    if (fs::is_directory(CLINGLITE_DEFAULT_CLING_DIR))
        return CLINGLITE_DEFAULT_CLING_DIR;
#endif
    return {};
}

static std::string probeIdaDir(const std::string& base) {
    if (base.empty()) return {};
    // Direct directory (e.g. IDADIR pointing to IDA installation)
    if (fs::exists(fs::path(base) / "libida.so")
        || fs::exists(fs::path(base) / "ida.dll")
        || fs::exists(fs::path(base) / "libida.dylib"))
        return base;
    // $IDASDK/src/bin layout (ida-cmake)
    fs::path srcBin = fs::path(base) / "src" / "bin";
    if (fs::is_directory(srcBin)) return srcBin.string();
    // $IDASDK/bin fallback
    fs::path bin = fs::path(base) / "bin";
    if (fs::is_directory(bin)) return bin.string();
    return {};
}

static std::string probeSdkInclude(const std::string& base) {
    if (base.empty()) return {};
    fs::path root(base);
    if (fs::is_regular_file(root / "include" / "pro.h"))
        return (root / "include").string();
    if (fs::is_regular_file(root / "src" / "include" / "pro.h"))
        return (root / "src" / "include").string();
    return {};
}

std::string resolveIdaDir() {
    // 1. IDADIR env takes precedence
    qstring env;
    if (qgetenv("IDADIR", &env) && !env.empty()) {
        std::string result = probeIdaDir(env.c_str());
        if (!result.empty()) return result;
        // IDADIR might point directly to the bin dir
        if (fs::is_directory(env.c_str())) return env.c_str();
    }
    // 2. IDASDK env → $IDASDK/src/bin
    if (qgetenv("IDASDK", &env) && !env.empty()) {
        std::string result = probeIdaDir(env.c_str());
        if (!result.empty()) return result;
    }
#ifdef CLINGLITE_DEFAULT_IDASDK
    // 3. Compile-time IDASDK → bin/
    {
        std::string result = probeIdaDir(CLINGLITE_DEFAULT_IDASDK);
        if (!result.empty()) return result;
    }
#endif
    return {};
}

std::string resolveSdkInclude() {
    qstring env;
    if (qgetenv("IDASDK", &env) && !env.empty()) {
        std::string result = probeSdkInclude(env.c_str());
        if (!result.empty()) return result;
    }
#ifdef CLINGLITE_DEFAULT_IDASDK
    {
        std::string result = probeSdkInclude(CLINGLITE_DEFAULT_IDASDK);
        if (!result.empty()) return result;
    }
#endif
    return {};
}

// ── Default PCH cache directory ──────────────────────────────────────────────

static fs::path defaultCacheDir() {
    return fs::path(get_user_idadir()) / "idacpp";
}

static std::string compilerFlagsKey(const std::vector<std::string>& flags) {
    std::ostringstream oss;
    for (const auto& flag : flags)
        oss << flag << '\n';
    return oss.str();
}

static std::string getExecutableIdentity() {
#ifdef _WIN32
    wchar_t exeBuf[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exeBuf, MAX_PATH) != 0)
        return fs::path(exeBuf).string();
    return "";
#elif defined(__APPLE__)
    uint32_t size = 0;
    if (_NSGetExecutablePath(nullptr, &size) != -1 || size == 0)
        return "";
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0)
        return "";
    buf.resize(std::strlen(buf.c_str()));
    std::error_code ec;
    fs::path resolved = fs::weakly_canonical(fs::path(buf), ec);
    return ec ? buf : resolved.string();
#else
    char exeBuf[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exeBuf, sizeof(exeBuf) - 1);
    if (len <= 0)
        return "";
    exeBuf[len] = '\0';
    return exeBuf;
#endif
}

static std::string buildPchCacheKey(
    const std::string& sdkInclude,
    const std::vector<std::string>& compilerFlags,
    const std::string& llvmDir)
{
    std::error_code ec;
    const fs::path sdkRoot = fs::weakly_canonical(fs::path(sdkInclude), ec);
    std::ostringstream oss;
    oss << "sdk=" << (ec ? sdkInclude : sdkRoot.string()) << '\n';

    const std::string exeIdentity = getExecutableIdentity();
    if (!exeIdentity.empty())
        oss << "exe=" << exeIdentity << '\n';
    else
        oss << "exe=<unknown>\n";

    oss << "llvmDir=" << llvmDir << '\n';
    oss << "flags=\n" << compilerFlagsKey(compilerFlags);
    oss << "std_headers=\n";
    for (const char* hdr : g_stdHeaders)
        oss << hdr << '\n';
    oss << "ida_headers=\n";
    for (const char* hdr : g_idaHeaders) {
        const fs::path headerPath = fs::path(sdkInclude) / hdr;
        std::error_code hdr_ec;
        oss << hdr;
        if (fs::exists(headerPath, hdr_ec)) {
            const auto stamp = fs::last_write_time(headerPath, hdr_ec);
            if (!hdr_ec) {
                oss << '|'
                    << std::chrono::duration_cast<std::chrono::nanoseconds>(
                           stamp.time_since_epoch()).count();
            }
        }
        oss << '\n';
    }
    return oss.str();
}

// ── preparePCH ───────────────────────────────────────────────────────────────

bool preparePCH(clinglite::Options& opts,
                const std::string& sdkInclude,
                const std::string& cacheDir_) {
    if (sdkInclude.empty()) return false;

    // Check that SDK include path exists
    std::error_code ec;
    if (!fs::is_directory(sdkInclude, ec)) return false;

    // Determine cache path, keyed by SDK include path hash for invalidation
    fs::path cacheDir = cacheDir_.empty() ? defaultCacheDir() : fs::path(cacheDir_);
    fs::create_directories(cacheDir, ec);

    // Cache key includes SDK path, host executable identity, compiler flags,
    // llvmDir, and the SDK header mtimes that contribute to the generated PCH.
    // This avoids stale reuse when the SDK or platform-specific options change.
    const std::string keyStr =
        buildPchCacheKey(sdkInclude, opts.compilerFlags, opts.llvmDir);
    const std::string exeIdentity = getExecutableIdentity();
    if (exeIdentity.empty())
        std::cerr << "Warning: PCH cache using fallback executable identity\n";
    size_t pathHash = std::hash<std::string>{}(keyStr);
    std::ostringstream hashBuf;
    hashBuf << std::hex << pathHash;
    std::string pchName = std::string("ida_sdk_") + hashBuf.str() + ".pch";
    fs::path pchPath = cacheDir / pchName;

    if (fs::exists(pchPath, ec)) {
        opts.pchPath = pchPath.string();
        opts.includePaths.push_back(sdkInclude);
        return true;
    }

    // Remove stale PCH files before generating a new one
    for (auto& entry : fs::directory_iterator(cacheDir, ec)) {
        if (!entry.is_regular_file())
            continue;
        const std::string name = entry.path().filename().string();
        if (name.size() > 8 && name.compare(0, 8, "ida_sdk_") == 0
            && name.size() > 4 && name.compare(name.size() - 4, 4, ".pch") == 0
            && entry.path() != pchPath) {
            std::error_code rm_ec;
            fs::remove(entry.path(), rm_ec);
        }
    }

    // Generate PCH on-the-fly
    std::cerr << "Generating IDA SDK PCH (first run, this takes a moment)...\n";

    // Create a temporary interpreter with -noruntime for PCH generation
    clinglite::Options pchOpts;
    pchOpts.args = {"pchgen", "-noruntime"};
    pchOpts.includePaths = {sdkInclude};
    pchOpts.compilerFlags = opts.compilerFlags;  // inherit platform defines
    // Suppress noisy IDA SDK warnings during PCH generation
    pchOpts.compilerFlags.push_back("-Wno-nullability-completeness");
    pchOpts.compilerFlags.push_back("-Wno-nontrivial-memcall");
    pchOpts.compilerFlags.push_back("-Wno-varargs");
    pchOpts.compilerFlags.push_back("-Wno-tautological-constant-out-of-range-compare");
    pchOpts.compilerFlags.push_back("-Wno-format");
    // Emit all declarations so inline function bodies (e.g. qstring::find)
    // are available for JIT after PCH load.
    pchOpts.compilerFlags.push_back("-femit-all-decls");

    pchOpts.llvmDir = opts.llvmDir;

    clinglite::Interpreter pchInterp(pchOpts);
    if (!pchInterp.isValid()) {
        std::cerr << "Warning: failed to create PCH generator interpreter\n";
        return false;
    }

    // Include C++ stdlib headers first
    int included = 0;
    for (const char* hdr : g_stdHeaders) {
        if (pchInterp.includeHeader(hdr))
            ++included;
    }

    // Include all IDA SDK headers
    for (const char* hdr : g_idaHeaders) {
        if (pchInterp.includeHeader(hdr))
            ++included;
        else
            std::cerr << "  skipped: " << hdr << "\n";
    }

    if (included == 0) {
        std::cerr << "Warning: no IDA headers could be included\n";
        return false;
    }

    // Serialize to PCH
    if (!pchInterp.generatePCH(pchPath.string())) {
        std::cerr << "Warning: PCH generation failed\n";
        return false;
    }

    std::cerr << "IDA SDK PCH cached: " << pchPath.string()
              << " (" << included << " headers)\n";

    opts.pchPath = pchPath.string();
    opts.includePaths.push_back(sdkInclude);
    return true;
}

// ── initIda ──────────────────────────────────────────────────────────────────

IdaSession initIda(clinglite::Interpreter& interp,
                   const std::string& idaDir,
                   const std::string& dbPath,
                   bool hasPch)
{
    IdaSession session;

    // If PCH was loaded, all IDA headers are already available
    if (hasPch) {
        std::cerr << "IDA SDK headers loaded from PCH\n";
        session.headersReady = true;
    }
    // Otherwise fall back to header-by-header inclusion
    else if (interp.execute("#include <pro.h>") == clinglite::ExecResult::Success) {
        for (const char* hdr : g_idaHeaders) {
            if (hdr == std::string("pro.h")) continue; // already included
            if (!interp.includeHeader(hdr))
                std::cerr << "Note: skipped " << hdr << "\n";
        }
        session.headersReady = true;
    }

    // Auto-load IDA libraries for JIT symbol resolution
    if (session.headersReady) {
#ifdef _WIN32
        const char* idaLibs[] = {"ida.dll", "idalib.dll"};
#elif defined(__APPLE__)
        const char* idaLibs[] = {"libida.dylib", "libidalib.dylib"};
#else
        const char* idaLibs[] = {"libida.so", "libidalib.so"};
#endif
        for (const char* lib : idaLibs) {
            bool loaded = false;
            if (!idaDir.empty()) {
                std::string fullPath = (fs::path(idaDir) / lib).string();
                loaded = (interp.loadLibrary(fullPath) ==
                          clinglite::ExecResult::Success);
            }
            if (!loaded)
                loaded = (interp.loadLibrary(lib) ==
                          clinglite::ExecResult::Success);
            std::cerr << (loaded ? "Loaded: " : "Warning: failed to load ")
                      << lib << "\n";
        }

        // Initialize IDA library runtime
        clinglite::Value initResult;
        if (interp.execute("init_library();", initResult) ==
            clinglite::ExecResult::Success) {
            if (initResult.hasValue() && initResult.asInt() == 0) {
                std::cerr << "IDA library initialized\n";
                interp.execute("enable_console_messages(true);");
                session.libInitialized = true;
            } else {
                std::cerr << "Warning: init_library() returned non-zero\n";
            }
        } else {
            std::cerr << "Warning: init_library() call failed\n";
        }

        // Register extlang
        if (session.libInitialized) {
            idacpp::init(&interp);
            idacpp::install();
        }

        // Open database if requested
        if (session.libInitialized && !dbPath.empty()) {
            std::string cmd =
                "open_database(\"" + clinglite::escapePath(dbPath) + "\", true);";
            if (interp.execute(cmd) == clinglite::ExecResult::Success) {
                interp.execute("auto_wait();");
                session.dbOpen = true;
                std::cerr << "Database opened: " << dbPath << "\n";
            } else {
                std::cerr << "Warning: failed to open database: " << dbPath << "\n";
            }
        }
    }

    return session;
}

// ── termIda ──────────────────────────────────────────────────────────────────

void termIda(clinglite::Interpreter& interp, const IdaSession& session) {
    if (session.libInitialized) {
        idacpp::remove();
        idacpp::term();
    }

    if (session.dbOpen)
        interp.execute("close_database(false);");
}

} // namespace idacpp
