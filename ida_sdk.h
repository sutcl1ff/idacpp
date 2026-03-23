// IDA SDK umbrella header for PCH generation
// Copyright (c) Elias Bachaalany
// SPDX-License-Identifier: MIT
//
// Generate PCH with clang from the cling build tree:
//   <cling-build>/bin/clang -x c++-header -std=c++17 \
//     -D__EA64__=1 -D__IDP__ -D__NT__ \
//     -I<idasdk>/include -o ida_sdk.pch ida_sdk.h
//
// On Linux replace -D__NT__ with -D__LINUX__
// On macOS replace -D__NT__ with -D__MAC__
//
// This curated list intentionally tracks the headers used by idacpp's
// PCH/completion generation. Keep it ordered and conservative: broad enough
// for useful SDK coverage, but not every public SDK header belongs here.

#include <pro.h>
#include <auto.hpp>
#include <allins.hpp>
#include <bitrange.hpp>
#include <bytes.hpp>
#include <compress.hpp>
#include <config.hpp>
#include <cvt64.hpp>
#include <dbg.hpp>
#include <demangle.hpp>
#include <diff3.hpp>
#include <dirtree.hpp>
#include <diskio.hpp>
#include <entry.hpp>
#include <err.h>
#include <exehdr.h>
#include <expr.hpp>
#include <fixup.hpp>
#include <fpro.h>
#include <frame.hpp>
#include <funcs.hpp>
#include <gdl.hpp>
#include <graph.hpp>
#include <help.h>
#include <hexrays.hpp>
#include <ida.hpp>
#include <ida_highlighter.hpp>
#include <idacfg.hpp>
#include <idalib.hpp>
#include <idd.hpp>
#include <idp.hpp>
#include <ieee.h>
#include <jumptable.hpp>
#include <kernwin.hpp>
#include <lex.hpp>
#include <libfuncs.hpp>
#include <lines.hpp>
#include <llong.hpp>
#include <loader.hpp>
#include <lumina.hpp>
#include <make_script_ns.hpp>
#include <md5.h>
#include <merge.hpp>
#include <mergemod.hpp>
#include <moves.hpp>
#include <nalt.hpp>
#include <name.hpp>
#include <netnode.hpp>
#include <network.hpp>
#include <offset.hpp>
#include <parsejson.hpp>
#include <problems.hpp>
#include <prodir.h>
#include <pronet.h>
#include <range.hpp>
#include <regfinder.hpp>
#include <registry.hpp>
#include <search.hpp>
#include <segment.hpp>
#include <segregs.hpp>
#include <srclang.hpp>
#include <strlist.hpp>
#include <tryblks.hpp>
#include <typeinf.hpp>
#include <ua.hpp>
#include <undo.hpp>
#include <workarounds.hpp>
#include <xref.hpp>
