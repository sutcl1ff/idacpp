// cpp_highlighter.cpp — C++ syntax highlighter implementation
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT

#include "cpp_highlighter.h"
#include "cpp_highlight_keywords.h"

#include <pro.h>
#include <expr.hpp>
#include <ida_highlighter.hpp>

#include <algorithm>
#include <cstring>

#include "ida_sdk_identifiers.inc"

namespace idacpp {

/// Highlights IDA SDK identifiers (functions, types, enumerators) from the
/// pre-generated table using binary search. Registered as a helper on the
/// syntax highlighter — IDA calls get_ident_color() for each identifier token.
struct ida_ident_highlighter_t : ida_syntax_highlighter_helper_t
{
    bool get_ident_color(syntax_highlight_style* out_color,
                         const qstring& ident) override
    {
        auto cmp = [](const char* a, const char* b) {
            return strcmp(a, b) < 0;
        };
        if (std::binary_search(g_idaSdkIdentifiers,
                               g_idaSdkIdentifiers + g_idaSdkIdentifierCount,
                               ident.c_str(), cmp)) {
            *out_color = HF_USER1;
            return true;
        }
        return false;
    }
};

static ida_ident_highlighter_t g_ident_highlighter;

struct cpp_highlighter_t : public ida_syntax_highlighter_t
{
    cpp_highlighter_t() : ida_syntax_highlighter_t()
    {
        // String and character literals
        open_strconst  = '"';
        close_strconst = '"';
        open_chrconst  = '\'';
        close_chrconst = '\'';
        escape_char    = '\\';

        // Preprocessor
        preprocessor_char = '#';

        // Colors
        text_color         = HF_DEFAULT;
        comment_color      = HF_COMMENT;
        string_color       = HF_STRING;
        preprocessor_color = HF_PREPROC;
        style              = HF_DEFAULT;
        literal_closer     = '\0';

        // Comments
        set_open_cmt("//");
        add_multi_line_comment("/*", "*/");

        // C++ language keywords
        add_keywords(highlight::kCppKeywords1, HF_KEYWORD1);

        // IDA SDK types
        add_keywords(highlight::kIdaTypes, HF_KEYWORD2);

        // IDA SDK constants
        add_keywords(highlight::kIdaConstants, HF_KEYWORD3);

        // IDA SDK functions/identifiers from generated table
        add_ida_syntax_highlighter_helper(&g_ident_highlighter);
    }
};

syntax_highlighter_t* getCppHighlighter()
{
    static cpp_highlighter_t g_cpp_highlighter;
    return &g_cpp_highlighter;
}

} // namespace idacpp
