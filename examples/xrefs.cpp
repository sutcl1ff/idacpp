// xrefs.cpp — enumerate cross-references to the first function
//
// Finds all code and data references pointing to the first function's
// entry address.  Demonstrates: xrefblk_t iteration, xref type names.

#include <kernwin.hpp>
#include <funcs.hpp>
#include <xref.hpp>
#include <name.hpp>

int main() {
    func_t *fn = get_next_func(0);
    if (fn == nullptr) {
        msg("No functions in the database.\n");
        return 0;
    }

    qstring func_name;
    get_func_name(&func_name, fn->start_ea);
    msg("Cross-references to %s (%a):\n",
        func_name.c_str(), fn->start_ea);

    int count = 0;
    xrefblk_t xb;
    for (bool ok = xb.first_to(fn->start_ea, XREF_ALL);
         ok;
         ok = xb.next_to()) {
        qstring from_name;
        get_name(&from_name, xb.from);
        msg("  [%3d] %a  %-20s  type=%d\n",
            count++,
            xb.from,
            from_name.c_str(),
            xb.type);
    }

    msg("Total: %d xrefs\n", count);
    return count;
}
