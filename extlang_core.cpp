// extlang_core.cpp — C++ expression language for IDA via clinglite
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Implements extlang_t callbacks that bridge IDA's expression evaluator
// to a clinglite::Interpreter instance.

#include "extlang_core.h"
#include "cpp_highlighter.h"

#include <pro.h>
#include <expr.hpp>

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace idacpp {

// Globals
static clinglite::Interpreter* g_interp = nullptr;
static std::unique_ptr<clinglite::Session> g_session;
static std::unique_ptr<clinglite::ScriptRunner> g_runner;
static RuntimeStatus g_status;

// Error helpers
/// Fill qstring errbuf from a std::string error message.
static void fillError(qstring* errbuf, const std::string& error,
                      const char* fallback) {
    if (!errbuf) return;
    if (!error.empty())
        *errbuf = error.c_str();
    else
        *errbuf = fallback;
}

static void setLastError(const std::string& error) {
    g_status.lastError = error;
}

static void clearLastError() {
    g_status.lastError.clear();
}


// Value conversion helpers
static std::string trim_ascii(std::string s) {
    const size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    const size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

static std::string strip_type_prefix(const std::string& s) {
    std::string trimmed = trim_ascii(s);
    if (trimmed.empty() || trimmed[0] != '(')
        return trimmed;

    const size_t close = trimmed.find(')');
    if (close == std::string::npos)
        return trimmed;

    const size_t value_pos = trimmed.find_first_not_of(" \t", close + 1);
    if (value_pos == std::string::npos)
        return trimmed;

    return trimmed.substr(value_pos);
}

static bool has_float_literal_chars(const std::string& s) {
    if (s.empty())
        return false;
    for (char c : s) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isdigit(uc))
            continue;
        switch (c) {
        case '+':
        case '-':
        case '.':
        case 'e':
        case 'E':
        case 'i':
        case 'I':
        case 'n':
        case 'N':
        case 'f':
        case 'F':
        case 'a':
        case 'A':
            continue;
        default:
            return false;
        }
    }
    return true;
}

static bool parse_quoted_string(std::string* out, const std::string& s) {
    if (s.size() < 2 || s.front() != '"' || s.back() != '"')
        return false;

    std::string decoded;
    decoded.reserve(s.size() - 2);
    for (size_t i = 1; i + 1 < s.size(); ++i) {
        char c = s[i];
        if (c == '\\' && i + 2 < s.size()) {
            char esc = s[++i];
            switch (esc) {
            case '\\': decoded.push_back('\\'); break;
            case '"':  decoded.push_back('"'); break;
            case 'n':  decoded.push_back('\n'); break;
            case 'r':  decoded.push_back('\r'); break;
            case 't':  decoded.push_back('\t'); break;
            case '0':  decoded.push_back('\0'); break;
            default:
                decoded.push_back('\\');
                decoded.push_back(esc);
                break;
            }
        } else {
            decoded.push_back(c);
        }
    }
    *out = std::move(decoded);
    return true;
}

static bool parse_int64(int64* out, const std::string& s) {
    if (s.empty())
        return false;

    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(s.c_str(), &end, 0);
    if (errno != 0 || end == s.c_str() || *end != '\0')
        return false;

    *out = static_cast<int64>(value);
    return true;
}

static bool parse_double(double* out, const std::string& s) {
    if (s.empty())
        return false;

    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(s.c_str(), &end);
    if (errno != 0 || end == s.c_str() || *end != '\0')
        return false;

    *out = value;
    return true;
}

static void release_idc_value(idc_value_t* value) {
    if (value == nullptr)
        return;
    free_idcv(value);
}

static bool assign_idc_string(
    idc_value_t* dst,
    const std::string& value)
{
    if (dst == nullptr)
        return true;
    dst->set_string(value.c_str(), value.size());
    return true;
}

static bool value_to_idc_impl(
    const clinglite::Value& src,
    idc_value_t* dst,
    qstring* errbuf = nullptr)
{
    if (dst == nullptr)
        return true;

    if (!src.hasValue()) {
        dst->set_long(0);
        return true;
    }

    const std::string printed = trim_ascii(src.toString());
    const std::string value = strip_type_prefix(printed);

    if (printed.find("(void *)") != std::string::npos
        || printed.find("(void*)") != std::string::npos) {
        dst->set_pvoid(src.asPtr());
        return true;
    }

    std::string decoded;
    if (parse_quoted_string(&decoded, value)) {
        return assign_idc_string(dst, decoded);
    }

    if (value == "true") {
        dst->set_long(1);
        return true;
    }
    if (value == "false") {
        dst->set_long(0);
        return true;
    }

    double as_double = 0.0;
    if (parse_double(&as_double, value)
        && (value.find('.') != std::string::npos
            || value.find('e') != std::string::npos
            || value.find('E') != std::string::npos
            || printed.find("(double)") != std::string::npos
            || printed.find("(float)") != std::string::npos)) {
        fpvalue_t fp;
        fp.clear();
        fp.from_double(as_double);
        dst->set_float(fp);
        return true;
    }

    int64 as_int = 0;
    if (parse_int64(&as_int, value)) {
        dst->set_int64(as_int);
        return true;
    }

    if (!printed.empty()) {
        return assign_idc_string(dst, printed);
    }

    fillError(errbuf, "Unsupported C++ result value", "Unsupported C++ result value");
    return false;
}

// Convert idc_value_t -> C++ literal string for building expressions
static bool idc_to_literal_impl(
    std::string* out,
    const idc_value_t& v,
    qstring* errbuf = nullptr)
{
    if (out == nullptr)
        return false;

    switch (v.vtype) {
    case VT_LONG:
        *out = std::to_string(v.num);
        return true;
    case VT_INT64:
        *out = std::to_string(v.i64) + "LL";
        return true;
    case VT_PVOID: {
        char buf[64];
        qsnprintf(buf, sizeof(buf), "(void*)0x%" FMT_64 "x",
                  static_cast<uint64>(reinterpret_cast<uintptr_t>(v.pvoid)));
        *out = buf;
        return true;
    }
    case VT_FLOAT: {
        double d = 0.0;
        if (v.e.to_double(&d) == REAL_ERROR_OK) {
            if (!std::isfinite(d)) {
                fillError(errbuf, "Non-finite IDC floating-point value is unsupported",
                          "Non-finite IDC floating-point value is unsupported");
                return false;
            }
            char buf[64];
            qsnprintf(buf, sizeof(buf), "%.17g", d);
            *out = buf;
        } else {
            char buf[128];
            v.e.to_str(buf, sizeof(buf), 0);
            *out = trim_ascii(buf);
            if (!has_float_literal_chars(*out)) {
                fillError(errbuf, "Unsupported IDC floating-point value",
                          "Unsupported IDC floating-point value");
                return false;
            }
        }
        // Ensure the literal parses as floating-point in C++
        if (out->find('.') == std::string::npos
            && out->find('e') == std::string::npos
            && out->find('E') == std::string::npos) {
            *out += ".0";
        }
        return true;
    }
    case VT_STR:
        *out = "\"" + clinglite::escapePath(v.c_str()) + "\"";
        return true;
    default:
        fillError(errbuf,
                  "Unsupported IDC value type for C++ call/assignment",
                  "Unsupported IDC value type for C++ call/assignment");
        return false;
    }
}

static bool ensure_session(std::string* error) {
    if (g_session && g_session->interp() != nullptr)
        return true;
    if (error != nullptr)
        *error = "C++ interpreter not initialized";
    setLastError("C++ interpreter not initialized");
    return false;
}

static bool eval_snippet_with_optional_wrap(
    clinglite::Session* session,
    const std::string& input,
    std::string* error)
{
    std::string code = input;
    const bool needsWrap = code.find('\n') != std::string::npos
                           && code.find('{') != 0;
    if (needsWrap)
        code = "{\n" + code + "\n}";
    return session->evalSnippet(code, error);
}

// extlang_t callbacks
static bool idaapi cpp_compile_expr(
    const char* name,
    ea_t /*current_ea*/,
    const char* expr,
    qstring* errbuf)
{
    if (!g_session) {
        if (errbuf) *errbuf = "C++ interpreter not initialized";
        return false;
    }

    // Wrap expression in a callable function: auto name() { return (expr); }
    std::string code = "auto ";
    code += name;
    code += "() { return (";
    code += expr;
    code += "); }";

    std::string error;
    if (!g_session->evalSnippet(code, &error)) {
        fillError(errbuf, error, "C++ compilation error");
        return false;
    }
    return true;
}

static bool idaapi cpp_compile_file(
    const char* file,
    const char* /*requested_namespace*/,
    qstring* errbuf)
{
    // Match IDAPython behavior: compile_file both compiles AND executes.
    // The IDA kernel does not call a separate "run" callback after this.
    FileExecOptions opts;
    opts.entrypoint = FileEntrypoint::Auto;
    std::string error;
    if (!execFile(file, opts, nullptr, &error)) {
        fillError(errbuf, error, "C++ file execution error");
        return false;
    }
    return true;
}

static bool idaapi cpp_call_func(
    idc_value_t* result,
    const char* name,
    const idc_value_t args[],
    size_t nargs,
    qstring* errbuf)
{
    if (!g_session) {
        if (errbuf) *errbuf = "C++ interpreter not initialized";
        return false;
    }

    // Build call expression: name(arg0, arg1, ...)
    std::string code = name;
    code += "(";
    for (size_t i = 0; i < nargs; ++i) {
        if (i > 0) code += ", ";
        std::string literal;
        if (!idc_to_literal_impl(&literal, args[i], errbuf))
            return false;
        code += literal;
    }
    code += ")";

    clinglite::Value val;
    std::string error;
    if (!g_session->evalExpr(code, val, &error)) {
        fillError(errbuf, error, "C++ call error");
        return false;
    }

    return result == nullptr || value_to_idc_impl(val, result, errbuf);
}

static bool idaapi cpp_eval_expr(
    idc_value_t* rv,
    ea_t /*current_ea*/,
    const char* expr,
    qstring* errbuf)
{
    if (!g_session) {
        if (errbuf) *errbuf = "C++ interpreter not initialized";
        return false;
    }

    clinglite::Value val;
    std::string error;
    if (!g_session->evalExpr(expr, val, &error)) {
        fillError(errbuf, error, "C++ evaluation error");
        return false;
    }

    return rv == nullptr || value_to_idc_impl(val, rv, errbuf);
}

static bool idaapi cpp_eval_snippet(
    const char* str,
    qstring* errbuf)
{
    if (!g_session) {
        if (errbuf) *errbuf = "C++ interpreter not initialized";
        return false;
    }

    std::string error;
    if (!eval_snippet_with_optional_wrap(g_session.get(), str, &error)) {
        fillError(errbuf, error, "C++ snippet error");
        return false;
    }
    return true;
}

static bool idaapi cpp_create_object(
    idc_value_t* /*result*/,
    const char* /*name*/,
    const idc_value_t /*args*/[],
    size_t /*nargs*/,
    qstring* errbuf)
{
    // C++ doesn't have IDA-style dynamic objects
    if (errbuf) *errbuf = "C++ does not support IDC objects";
    return false;
}

static bool idaapi cpp_get_attr(
    idc_value_t* result,
    const idc_value_t* /*obj*/,
    const char* attr)
{
    if (!g_session)
        return false;

    // If attr is nullptr or empty, return the language name
    if (attr == nullptr || attr[0] == '\0') {
        if (result)
            return assign_idc_string(result, "c++");
        return true;
    }

    // Evaluate the variable name
    clinglite::Value val;
    if (!g_session->evalExpr(attr, val))
        return false;

    return result == nullptr || value_to_idc_impl(val, result);
}

static bool idaapi cpp_set_attr(
    idc_value_t* /*obj*/,
    const char* attr,
    const idc_value_t& value)
{
    if (!g_session || attr == nullptr || attr[0] == '\0')
        return false;

    // Build assignment: attr = literal_value;
    std::string code = attr;
    code += " = ";
    std::string literal;
    if (!idc_to_literal_impl(&literal, value))
        return false;
    code += literal;
    code += ";";

    return g_session->evalSnippet(code);
}

static bool idaapi cpp_call_method(
    idc_value_t* /*result*/,
    const idc_value_t* /*obj*/,
    const char* /*name*/,
    const idc_value_t /*args*/[],
    size_t /*nargs*/,
    qstring* errbuf)
{
    if (errbuf) *errbuf = "C++ does not support IDC method calls";
    return false;
}

static bool idaapi cpp_load_procmod(
    idc_value_t* /*procobj*/,
    const char* /*path*/,
    qstring* /*errbuf*/)
{
    return false;
}

static bool idaapi cpp_unload_procmod(
    const char* /*path*/,
    qstring* /*errbuf*/)
{
    return false;
}

// extlang_t struct
static extlang_t extlang_cpp = {
    sizeof(extlang_t),
    0,                    // flags
    0,                    // refcnt
    "C++",                // name
    "cpp",                // fileext
    nullptr,              // highlighter (set lazily in install())
    cpp_compile_expr,
    cpp_compile_file,
    cpp_call_func,
    cpp_eval_expr,
    cpp_eval_snippet,
    cpp_create_object,
    cpp_get_attr,
    cpp_set_attr,
    cpp_call_method,
    cpp_load_procmod,
    cpp_unload_procmod,
};

// Public API
void prepareOptions(clinglite::Options& opts) {
    // IDA SDK platform defines — same as used to build the plugin/PCH
    opts.compilerFlags.push_back("-D__EA64__=1");
    opts.compilerFlags.push_back("-D__IDP__");
#ifdef __NT__
    opts.compilerFlags.push_back("-D__NT__");
#elif defined(__LINUX__)
    opts.compilerFlags.push_back("-D__LINUX__");
#elif defined(__MAC__)
    opts.compilerFlags.push_back("-D__MAC__");
#endif
    opts.compilerFlags.push_back("-std=c++2b");
}

void init(clinglite::Interpreter* interp) {
    g_interp = interp;
    g_session = std::make_unique<clinglite::Session>(interp);
    g_runner = std::make_unique<clinglite::ScriptRunner>(g_session.get());
    g_runner->setNamespacePrefix("idacpp_exec_file_");
    g_runner->setRuntimeNamespace("idacpp_runtime");
    g_status = RuntimeStatus{};
    g_status.sessionReady = g_session != nullptr;
}

clinglite::Session* getSession() {
    return g_session.get();
}

bool compileFile(
    const std::string& file,
    const char* requestedNamespace,
    std::string* error)
{
    if (!ensure_session(error))
        return false;
    g_status.sessionReady = true;

    if (!g_runner->compileFile(file, requestedNamespace, error)) {
        auto rs = g_runner->status();
        setLastError(rs.lastError);
        g_status.lastFileExecuted = rs.lastFileExecuted;
        g_status.lastFileNamespace = rs.lastFileNamespace;
        return false;
    }

    auto rs = g_runner->status();
    g_status.lastFileExecuted = rs.lastFileExecuted;
    g_status.lastFileNamespace = rs.lastFileNamespace;
    g_status.lastEntrypointChosen.clear();
    clearLastError();
    return true;
}

bool execFile(
    const std::string& file,
    const FileExecOptions& options,
    clinglite::Value* result,
    std::string* error)
{
    if (!ensure_session(error))
        return false;
    g_status.sessionReady = true;

    clinglite::FileExecOptions clOpts;
    clOpts.args = options.args;

    switch (options.entrypoint) {
    case FileEntrypoint::Auto:
        clOpts.entrypointGroups = {clinglite::standardMainCandidates()};
        clOpts.allowNoEntrypoint = true;
        break;
    case FileEntrypoint::Main:
        clOpts.entrypointGroups = {clinglite::standardMainCandidates()};
        clOpts.allowNoEntrypoint = false;
        break;
    case FileEntrypoint::None:
        clOpts.allowNoEntrypoint = true;
        break;
    }

    if (!g_runner->execFile(file, clOpts, result, error)) {
        auto rs = g_runner->status();
        setLastError(rs.lastError);
        g_status.lastFileExecuted = rs.lastFileExecuted;
        g_status.lastFileNamespace = rs.lastFileNamespace;
        g_status.lastEntrypointChosen = rs.lastEntrypointChosen;
        return false;
    }

    auto rs = g_runner->status();
    g_status.lastFileExecuted = rs.lastFileExecuted;
    g_status.lastFileNamespace = rs.lastFileNamespace;
    g_status.lastEntrypointChosen = rs.lastEntrypointChosen;
    clearLastError();
    return true;
}

bool valueToIdc(
    const clinglite::Value& src,
    idc_value_t* dst,
    qstring* errbuf)
{
    return value_to_idc_impl(src, dst, errbuf);
}

bool idcToLiteral(
    std::string* out,
    const idc_value_t& v,
    qstring* errbuf)
{
    return idc_to_literal_impl(out, v, errbuf);
}

RuntimeStatus getRuntimeStatus() {
    return g_status;
}

void resetRuntimeStatus() {
    const bool sessionReady = g_session != nullptr;
    const bool extlangInstalled = g_status.extlangInstalled;
    g_status = RuntimeStatus{};
    g_status.sessionReady = sessionReady;
    g_status.extlangInstalled = extlangInstalled;
    if (g_runner)
        g_runner->resetStatus();
}

void install() {
    extlang_cpp.highlighter = idacpp::getCppHighlighter();
    install_extlang(&extlang_cpp);
    g_status.extlangInstalled = true;
}

void remove() {
    remove_extlang(&extlang_cpp);
    g_status.extlangInstalled = false;
}

const extlang_t* getExtlang() {
    return &extlang_cpp;
}

void term() {
    g_runner.reset();
    g_session.reset();
    g_interp = nullptr;
    g_status = RuntimeStatus{};
}

} // namespace idacpp
