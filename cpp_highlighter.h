// cpp_highlighter.h — C++ syntax highlighter for IDA extlang_t
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Returns a pointer to a static ida_syntax_highlighter_t configured for C++.
// Requires IDA kernel (code_highlight_block symbol).

#pragma once

struct syntax_highlighter_t;

namespace idacpp {

/// Returns the C++ syntax highlighter instance for use in extlang_t.
syntax_highlighter_t* getCppHighlighter();

} // namespace idacpp
