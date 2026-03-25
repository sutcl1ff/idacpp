// Print database metadata.
using namespace ida;

int main() {
    auto path = database::input_file_path().value_or("(unknown)");
    auto fmt  = database::file_type_name().value_or("(unknown)");
    auto proc = database::processor_name().value_or("(unknown)");
    auto bits = database::address_bitness().value_or(0);
    auto md5  = database::input_md5().value_or("(unavailable)");
    auto base = database::image_base().value_or(0);

    msg("Input:     %s\n", path.c_str());
    msg("Format:    %s\n", fmt.c_str());
    msg("Processor: %s (%d-bit)\n", proc.c_str(), bits);
    msg("Image base: %a\n", base);
    msg("MD5:       %s\n", md5.c_str());
    msg("Functions: %zu\n", function::count().value_or(0));
    msg("Segments:  %zu\n", segment::count().value_or(0));
    return 0;
}
