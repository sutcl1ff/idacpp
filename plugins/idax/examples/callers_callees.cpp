// Show callers and callees of the first function.
using namespace ida;

int main() {
    auto fn = function::by_index(0);
    if (!fn) {
        msg("No functions found\n");
        return 1;
    }

    auto addr = fn->start();
    auto nm = function::name_at(addr).value_or("???");
    msg("=== %s (%a) ===\n", nm.c_str(), addr);

    if (auto callers = function::callers(addr); callers && !callers->empty()) {
        msg("Callers (%zu):\n", callers->size());
        for (auto ea : *callers)
            msg("  %a  %s\n", ea, function::name_at(ea).value_or("???").c_str());
    } else {
        msg("Callers: (none)\n");
    }

    if (auto callees = function::callees(addr); callees && !callees->empty()) {
        msg("Callees (%zu):\n", callees->size());
        for (auto ea : *callees)
            msg("  %a  %s\n", ea, function::name_at(ea).value_or("???").c_str());
    } else {
        msg("Callees: (none)\n");
    }
    return 0;
}
