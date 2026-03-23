// decompile_first.cpp — decompile the first function using Hex-Rays
//
// Adapted from the IDA SDK vds1 sample.  Requires Hex-Rays decompiler.
// If Hex-Rays is not available, prints a message and returns gracefully.
// Demonstrates: init_hexrays_plugin(), decompile(), pseudocode extraction

#include <kernwin.hpp>
#include <funcs.hpp>
#include <hexrays.hpp>

int main() {
    if (!init_hexrays_plugin()) {
        msg("Hex-Rays decompiler not available.\n");
        return -1;
    }

    func_t *fn = get_next_func(0);
    if (fn == nullptr) {
        msg("No functions in the database.\n");
        term_hexrays_plugin();
        return 0;
    }

    msg("Decompiling function at %a...\n", fn->start_ea);

    hexrays_failure_t hf;
    cfuncptr_t cfunc = decompile(fn, &hf, DECOMP_WARNINGS);
    if (cfunc == nullptr) {
        msg("Decompilation failed: %s\n", hf.desc().c_str());
        term_hexrays_plugin();
        return -1;
    }

    const strvec_t &sv = cfunc->get_pseudocode();
    msg("Pseudocode (%d lines):\n", (int)sv.size());
    for (int i = 0; i < (int)sv.size(); ++i) {
        qstring clean;
        tag_remove(&clean, sv[i].line);
        msg("  %s\n", clean.c_str());
    }

    term_hexrays_plugin();
    return (int)sv.size();
}
