// list_functions.cpp — enumerate all functions in the database
//
// Prints each function's address and name.
// Demonstrates: get_func_qty(), get_next_func(), get_func_name()

#include <kernwin.hpp>
#include <funcs.hpp>
#include <name.hpp>

int main() {
    int count = get_func_qty();
    msg("Functions: %d\n", count);

    func_t *fn = get_next_func(0);
    int i = 0;
    while (fn != nullptr) {
        qstring name;
        get_func_name(&name, fn->start_ea);
        msg("  [%4d] %a  %s\n",
            i++, fn->start_ea, name.c_str());
        fn = get_next_func(fn->start_ea);
    }

    return count;
}
