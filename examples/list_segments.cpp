// list_segments.cpp — enumerate all segments in the database
//
// Prints each segment's name, address range, and size.
// Demonstrates: get_segm_qty(), get_first_seg(), get_next_seg(), get_segm_name()

#include <kernwin.hpp>
#include <segment.hpp>

int main() {
    int count = get_segm_qty();
    msg("Segments: %d\n", count);

    segment_t *seg = get_first_seg();
    int i = 0;
    while (seg != nullptr) {
        qstring name;
        get_segm_name(&name, seg);
        msg("  [%2d] %-16s  %a - %a  (%llu bytes)\n",
            i++, name.c_str(),
            seg->start_ea,
            seg->end_ea,
            (unsigned long long)(seg->end_ea - seg->start_ea));
        seg = get_next_seg(seg->start_ea);
    }

    return count;
}
