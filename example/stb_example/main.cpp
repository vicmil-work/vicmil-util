#include "util_std.hpp"
#include "util_stb.hpp"
#include "util_obj_loader.hpp"
#include "noto_sans_mono_jp.hpp"
#include "util_js.hpp"

int main() {
    vicmil::FontLoader font_loader;
    font_loader.load_font_from_memory(NOTO_SANS_MONO_JP_HPP_data, NOTO_SANS_MONO_JP_HPP_size);
    int character_unicode = vicmil::utf8ToUnicodeCodePoints("本")[0];
    vicmil::ImageRGBA_UChar char_image = font_loader.get_character_image_rgba(character_unicode);
    std::vector<unsigned char> png_bytes = char_image.to_png_as_bytes();
    vicmil::download_file("char_img.png", png_bytes);
    return 0;
}