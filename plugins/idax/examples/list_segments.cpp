// List all segments with address ranges and permissions.
using namespace ida;

int main() {
    auto n = segment::count().value_or(0);
    msg("%zu segments:\n", n);

    for (const auto& seg : segment::all()) {
        auto perm = seg.permissions();
        msg("  %a-%a  %-12s  %c%c%c\n",
               seg.start(), seg.end(), seg.name().c_str(),
               perm.read    ? 'R' : '-',
               perm.write   ? 'W' : '-',
               perm.execute ? 'X' : '-');
    }
    return 0;
}
