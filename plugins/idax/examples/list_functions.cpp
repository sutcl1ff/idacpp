// List all functions with addresses and names.
using namespace ida;

int main() {
    auto n = function::count().value_or(0);
    msg("%zu functions:\n", n);

    for (const auto& fn : function::all()) {
        auto addr = fn.start();
        auto nm = function::name_at(addr).value_or("???");
        msg("  %a  %s\n", addr, nm.c_str());
    }
    return 0;
}
