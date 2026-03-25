// Show all cross-references to the first function.
using namespace ida;

int main() {
    auto fn = function::by_index(0);
    if (!fn) {
        msg("No functions found\n");
        return 1;
    }

    auto addr = fn->start();
    auto nm = function::name_at(addr).value_or("???");
    msg("Xrefs to %s (%a):\n", nm.c_str(), addr);

    auto refs = xref::refs_to(addr);
    if (!refs || refs->empty()) {
        msg("  (none)\n");
        return 0;
    }

    for (const auto& ref : *refs) {
        auto from_name = name::get(ref.from).value_or("");
        msg("  %a  %s\n", ref.from,
               from_name.empty() ? "(unnamed)" : from_name.c_str());
    }
    return 0;
}
