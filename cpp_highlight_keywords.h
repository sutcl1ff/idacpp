// cpp_highlight_keywords.h — C++ and IDA SDK keyword lists for syntax highlighting
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Pure C++ header with no IDA dependency. Pipe-delimited strings compatible
// with ida_syntax_highlighter_t::add_keywords().

#pragma once

namespace idacpp { namespace highlight {

// C++ language keywords (HF_KEYWORD1)
// Covers C++17 + C++20/23 additions + alternative tokens
constexpr const char* kCppKeywords1 =
    "alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|"
    "case|catch|char|char8_t|char16_t|char32_t|class|compl|concept|"
    "const|consteval|constexpr|constinit|const_cast|continue|"
    "co_await|co_return|co_yield|"
    "decltype|default|delete|do|double|dynamic_cast|"
    "else|enum|explicit|export|extern|"
    "false|float|for|friend|goto|"
    "if|inline|int|long|mutable|"
    "namespace|new|noexcept|not|not_eq|nullptr|"
    "operator|or|or_eq|"
    "private|protected|public|"
    "register|reinterpret_cast|requires|return|"
    "short|signed|sizeof|static|static_assert|static_cast|"
    "struct|switch|"
    "template|this|thread_local|throw|true|try|typedef|typeid|typename|"
    "union|unsigned|using|virtual|void|volatile|while|xor|xor_eq";

// IDA SDK types (HF_KEYWORD2)
constexpr const char* kIdaTypes =
    "ea_t|sel_t|tid_t|flags_t|uval_t|sval_t|asize_t|adiff_t|"
    "func_t|segment_t|struc_t|member_t|insn_t|op_t|"
    "tinfo_t|til_t|argloc_t|funcarg_t|udt_member_t|"
    "cfunc_t|cinsn_t|cexpr_t|citem_t|ctree_item_t|"
    "qstring|qvector|qlist|qstack|"
    "linput_t|loader_t|idainfo|"
    "netnode|"
    "fixup_info_t|refinfo_t|xrefblk_t|"
    "bytevec_t|relobj_t|eavec_t|"
    "idc_value_t|extlang_t|"
    "mbl_array_t|minsn_t|mop_t|mblock_t|"
    "graph_visitor_t|abstract_graph_t";

// IDA SDK constants (HF_KEYWORD3)
constexpr const char* kIdaConstants =
    "BADADDR|BADSEL|MAXADDR|MAXSTR|"
    "SEGPERM_EXEC|SEGPERM_WRITE|SEGPERM_READ|"
    "SFL_HIDDEN|SFL_DEBUG|SFL_LOADER|SFL_COMDAT|"
    "cot_num|cot_var|cot_call|cot_memref|cot_idx|"
    "cit_if|cit_for|cit_while|cit_do|cit_switch|cit_return|cit_block|"
    "fl_CF|fl_CN|fl_JF|fl_JN|fl_F|"
    "dt_byte|dt_word|dt_dword|dt_qword|dt_float|dt_double";

}} // namespace idacpp::highlight
