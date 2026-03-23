// hello.cpp — minimal idacpp script
//
// Usage from IDA's C++ CLI tab:
//   .L hello.cpp
//   main()
//
// Or load + run in one step:
//   .x hello.cpp

#include <kernwin.hpp>
#include <funcs.hpp>
#include <segment.hpp>

int main() {
    msg("Hello from idacpp!\n");
    msg("  Functions: %d\n", get_func_qty());
    msg("  Segments:  %d\n", get_segm_qty());
    return 0;
}
