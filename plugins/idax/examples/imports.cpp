// List imported modules and their symbols.
using namespace ida;

int main() {
    auto modules = database::import_modules();
    if (!modules || modules->empty()) {
        msg("No imports found\n");
        return 0;
    }

    msg("%zu import modules:\n", modules->size());
    for (const auto& mod : *modules) {
        msg("\n  %s (%zu symbols):\n", mod.name.c_str(), mod.symbols.size());
        for (const auto& sym : mod.symbols) {
            if (sym.ordinal > 0 && sym.name.empty())
                msg("    %a  ordinal #%u\n", sym.address, sym.ordinal);
            else
                msg("    %a  %s\n", sym.address, sym.name.c_str());
        }
    }
    return 0;
}
