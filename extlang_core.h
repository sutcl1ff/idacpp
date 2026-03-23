// extlang_core.h — C++ expression language for IDA
//
// Bridges clinglite::Interpreter to IDA's extlang_t interface.
// This header is safe to include from files that don't include pro.h.

#pragma once

#include <clinglite/clinglite.h>

template <class T> class _qstring;
typedef _qstring<char> qstring;
class idc_value_t;
struct extlang_t;

namespace idacpp {

enum class FileEntrypoint {
    Auto,
    Main,
    None,
};

struct FileExecOptions {
    std::vector<std::string> args;
    FileEntrypoint entrypoint = FileEntrypoint::Auto;
};

/// Snapshot of the runtime lifecycle.
/// Fields sessionReady, extlangInstalled, lastFile*, lastEntrypoint*, and
/// lastError are managed by extlang_core.
/// Fields rcLoaded and startupScriptLoaded are set by the host (plugin or
/// standalone) after their respective operations complete.
struct RuntimeStatus {
    bool sessionReady = false;
    bool extlangInstalled = false;
    bool rcLoaded = false;
    bool startupScriptLoaded = false;
    std::string lastFileExecuted;
    std::string lastFileNamespace;
    std::string lastEntrypointChosen;
    std::string lastError;
};

/// Set up IDA SDK PCH in interpreter options.
void prepareOptions(clinglite::Options& opts);

/// Initialize and register the C++ extlang with IDA.
void init(clinglite::Interpreter* interp);

/// Get the Session used by the extlang (for CLI tab, etc.).
/// Returns nullptr if not initialized.
clinglite::Session* getSession();

/// Compile and load a .cpp file into the active session.
/// If requestedNamespace is non-empty, the file is wrapped in that namespace.
bool compileFile(
    const std::string& file,
    const char* requestedNamespace = nullptr,
    std::string* error = nullptr);

/// Load a .cpp file and optionally invoke a conventional entrypoint.
/// When entrypoint is Auto, probes for main(argc, argv), then main().
bool execFile(
    const std::string& file,
    const FileExecOptions& options = {},
    clinglite::Value* result = nullptr,
    std::string* error = nullptr);

/// Convert a Cling value to IDC.
bool valueToIdc(
    const clinglite::Value& src,
    idc_value_t* dst,
    qstring* errbuf = nullptr);

/// Convert an IDC value into a C++ literal string.
bool idcToLiteral(
    std::string* out,
    const idc_value_t& v,
    qstring* errbuf = nullptr);

/// Return a snapshot of the runtime status.
RuntimeStatus getRuntimeStatus();

/// Reset the runtime status snapshot.
void resetRuntimeStatus();

/// Install the extlang (calls install_extlang).
void install();

/// Unregister and clean up the extlang.
void remove();

/// Get the extlang_t pointer for direct callback invocation.
const extlang_t* getExtlang();

/// Cleanup interpreter reference.
void term();

} // namespace idacpp
