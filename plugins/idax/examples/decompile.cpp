// Decompile the first function (requires Hex-Rays).
using namespace ida;

int main() {
    if (auto avail = decompiler::available(); !avail || !*avail) {
        msg("Hex-Rays decompiler not available\n");
        return -1;
    }

    auto fn = function::by_index(0);
    if (!fn) {
        msg("No functions found\n");
        return 1;
    }

    auto addr = fn->start();
    auto nm = function::name_at(addr).value_or("???");
    msg("Decompiling %s (%a)...\n\n", nm.c_str(), addr);

    auto result = decompiler::decompile(addr);
    if (!result) {
        msg("Decompilation failed\n");
        return 1;
    }

    auto code = result->pseudocode().value_or("(empty)");
    msg("%s\n", code.c_str());
    return 0;
}
