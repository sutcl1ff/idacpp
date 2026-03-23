// idacpp_idgen — generate sorted IDA SDK identifier table
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Usage: idacpp_idgen [options] -o output.inc header1.h header2.h ...
//   -I <path>         Add include path
//   -D <macro>        Add define
//   -std=<std>        C++ standard (default: c++2b)
//   -o <path>         Output .inc file (C++ header with sorted table)
//   --sdk-dir <path>  IDA SDK include dir (for source filtering)
//   --libs lib1 lib2  Shared libraries to extract exports from
//   --help            Show usage
//
// Combines AST declarations from headers + library exports into a single
// sorted, deduplicated C++ table for instant Tab completion and syntax
// highlighting in the idacpp plugin.

#include <clinglite/clinglite.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

static void usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [options] -o output.inc header1.h header2.h ...\n"
        "  -I <path>         Add include path\n"
        "  -D <macro>        Add define\n"
        "  -std=<std>        C++ standard (default: c++2b)\n"
        "  -o <path>         Output .inc file\n"
        "  --sdk-dir <path>  IDA SDK include dir (source filter)\n"
        "  --libs lib1 ...   Shared libraries for export extraction\n"
        "  --help            Show usage\n",
        argv0);
}

/// Returns true if the identifier looks useful for completion.
/// Filters out mangled C++ names, very short names, and internal prefixes.
static bool isUsefulIdentifier(const std::string& name) {
    if (name.size() < 2) return false;

    // Skip C++ mangled names (start with _Z on Itanium ABI)
    if (name.size() > 2 && name[0] == '_' && name[1] == 'Z')
        return false;

    // Skip names starting with double underscore (compiler internals)
    if (name.size() > 2 && name[0] == '_' && name[1] == '_')
        return false;

    // Skip names that are purely numeric or start with digit
    if (std::isdigit(static_cast<unsigned char>(name[0])))
        return false;

    // Must contain only valid C/C++ identifier characters
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
    }

    return true;
}

int main(int argc, char** argv) {
    std::string outputPath;
    std::string sdkDir;
    std::vector<std::string> includePaths;
    std::vector<std::string> compilerFlags;
    std::vector<std::string> headers;
    std::vector<std::string> libs;
    std::string cxxStd = "-std=c++2b";

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--sdk-dir" && i + 1 < argc) {
            sdkDir = argv[++i];
        } else if (arg == "--libs") {
            while (i + 1 < argc && argv[i + 1][0] != '-')
                libs.push_back(argv[++i]);
        } else if (arg.substr(0, 2) == "-I") {
            if (arg.size() > 2)
                includePaths.push_back(arg.substr(2));
            else if (i + 1 < argc)
                includePaths.push_back(argv[++i]);
        } else if (arg.substr(0, 2) == "-D") {
            if (arg.size() > 2)
                compilerFlags.push_back(arg);
            else if (i + 1 < argc)
                compilerFlags.push_back(std::string("-D") + argv[++i]);
        } else if (arg.substr(0, 5) == "-std=") {
            cxxStd = arg;
        } else if (arg[0] != '-') {
            headers.push_back(arg);
        } else {
            fprintf(stderr, "Unknown option: %s\n", arg.c_str());
            usage(argv[0]);
            return 1;
        }
    }

    if (outputPath.empty()) {
        fprintf(stderr, "Error: -o output.inc is required\n");
        usage(argv[0]);
        return 1;
    }

    // Collect all identifiers into a set for automatic dedup + sort
    std::set<std::string> identifiers;

    // ── AST declarations from headers ───────────────────────────────────
    if (!headers.empty()) {
        clinglite::Environment env(argv[0]);

        clinglite::Options opts;
        opts.args = {"idacpp_idgen", "-noruntime"};
        opts.includePaths = includePaths;
        compilerFlags.push_back(cxxStd);
        // Suppress noisy IDA SDK warnings
        compilerFlags.push_back("-Wno-nullability-completeness");
        compilerFlags.push_back("-Wno-nontrivial-memcall");
        compilerFlags.push_back("-Wno-varargs");
        compilerFlags.push_back("-Wno-tautological-constant-out-of-range-compare");
        compilerFlags.push_back("-Wno-format");
        opts.compilerFlags = compilerFlags;

        clinglite::Interpreter interp(opts);
        if (!interp.isValid()) {
            fprintf(stderr, "Error: failed to initialize interpreter\n");
            return 1;
        }

        // Pre-include C++ stdlib headers that IDA SDK depends on transitively.
        // macOS libc++ in C++2b mode de-transitivizes headers, so std::is_pod
        // (used by pro.h:2183) won't be found through <memory>/<string> alone.
        static const char* stdPreIncludes[] = {
            "cstdio", "cstdlib", "cstring", "cstdint",
            "string", "vector", "map", "set",
            "memory", "functional", "type_traits",
        };
        for (const char* hdr : stdPreIncludes)
            interp.includeHeader(hdr);

        int included = 0;
        for (const auto& hdr : headers) {
            if (interp.includeHeader(hdr))
                ++included;
            else
                fprintf(stderr, "Warning: failed to include <%s>\n",
                        hdr.c_str());
        }
        fprintf(stderr, "Included %d/%zu headers\n", included, headers.size());

        auto decls = interp.enumerateDeclarations(sdkDir);
        size_t filtered = 0;
        for (auto& name : decls) {
            if (isUsefulIdentifier(name)) {
                identifiers.insert(std::move(name));
            } else {
                ++filtered;
            }
        }
        fprintf(stderr, "Declarations: %zu kept, %zu filtered\n",
                decls.size() - filtered, filtered);
    }

    // ── Library exports ─────────────────────────────────────────────────
    for (const auto& lib : libs) {
        auto exports =
            clinglite::Interpreter::enumerateLibraryExports(lib);
        size_t filtered = 0;
        for (auto& name : exports) {
            if (isUsefulIdentifier(name)) {
                identifiers.insert(std::move(name));
            } else {
                ++filtered;
            }
        }
        fprintf(stderr, "Exports from %s: %zu kept, %zu filtered\n",
                lib.c_str(), exports.size() - filtered, filtered);
    }

    fprintf(stderr, "Total unique identifiers: %zu\n", identifiers.size());

    // ── Write .inc header ───────────────────────────────────────────────
    FILE* out = fopen(outputPath.c_str(), "w");
    if (!out) {
        fprintf(stderr, "Error: cannot open %s for writing\n",
                outputPath.c_str());
        return 1;
    }

    fprintf(out,
        "// ida_sdk_identifiers.inc — Pre-computed IDA SDK identifier table\n"
        "// Generated by idacpp_idgen from SDK headers + library exports.\n"
        "//\n"
        "// This table is used for instant Tab completion and syntax highlighting\n"
        "// instead of Cling's live code completer (codeCompleteWithContext), which\n"
        "// performs full Clang Sema analysis per keystroke. Pre-computing the table\n"
        "// at build time gives O(log n) prefix lookup with zero runtime overhead.\n"
        "//\n"
        "// DO NOT EDIT — regenerate by deleting this file and rebuilding.\n"
        "\n"
        "static const char* const g_idaSdkIdentifiers[] = {\n");

    for (const auto& id : identifiers)
        fprintf(out, "    \"%s\",\n", id.c_str());

    fprintf(out,
        "};\n"
        "static constexpr size_t g_idaSdkIdentifierCount =\n"
        "    sizeof(g_idaSdkIdentifiers) / sizeof(g_idaSdkIdentifiers[0]);\n");

    fclose(out);

    fprintf(stderr, "Written %zu identifiers to %s\n",
            identifiers.size(), outputPath.c_str());
    return 0;
}
