// Find all call instructions in the first function.
using namespace ida;

int main() {
    auto fn = function::by_index(0);
    if (!fn) {
        msg("No functions found\n");
        return 1;
    }

    auto addr = fn->start();
    auto nm = function::name_at(addr).value_or("???");
    msg("Call instructions in %s:\n", nm.c_str());

    int count = 0;
    for (auto ea : *function::code_addresses(addr)) {
        if (instruction::is_call(ea)) {
            auto text = instruction::text(ea).value_or("???");
            auto targets = instruction::call_targets(ea);
            msg("  %a  %s", ea, text.c_str());
            if (targets && !targets->empty()) {
                auto tgt_name = name::get((*targets)[0]).value_or("");
                if (!tgt_name.empty())
                    msg("  -> %s", tgt_name.c_str());
            }
            msg("\n");
            ++count;
        }
    }
    msg("(%d calls)\n", count);
    return 0;
}
