
// ============================================================
//                           Include
// ============================================================

#pragma once
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb/stb_truetype.h"

namespace vicmil {
struct ColorRGBA_UChar {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;
    ColorRGBA_UChar(){}
    ColorRGBA_UChar(int r_, int g_, int b_, int a_) {
        r = r_;
        g = g_;
        b = b_;
        a = a_;
    }
    std::string to_string() {
        return vicmil::vec_to_str<int>({r, g, b});
    }
    bool operator==(const ColorRGBA_UChar& other) const {
        return  r == other.r && 
                g == other.g && 
                b == other.b && 
                a == other.a;
    }
};

// ============================================================
//                           Loading images
// ============================================================

 /**
 * General template class for rectangle
*/
template<class T>
class RectT {
    public:
    T x = 0;
    T y = 0;
    T w = 0;
    T h = 0;
    RectT() {}
    RectT(T x_, T y_, T w_, T h_) {
        x = x_;
        y = y_;
        w = w_;
        h = h_;
    }
    T min_x() const {
        return x;
    }
    T max_x() const {
        return x + w;
    }
    T min_y() const {
        return y;
    }
    T max_y() const {
        return y + h;
    }
    T center_x() const {
        return x + (w / 2.0);
    }
    T center_y() const {
        return y + (h / 2.0);
    }
    /**
     * Determine if a value is in range
     * Returns true if min_v <= v <= max_v
    */
    template<class T2>
    static inline bool _in_range(T2 v, T2 min_v, T2 max_v) {
        if(v < min_v) {
            return false;
        }
        if(v > max_v) {
            return false;
        }
        return true;
    }
    bool is_inside_rect(T x_, T y_) const {
        if(!_in_range<T>(x_, x, x + w)) {
            return false;
        }
        if(!_in_range<T>(y_, y, y + h)) {
            return false;
        }
        return true;
    }
    std::string to_string() const {
        return "x: " + std::to_string(x) + 
                "   y: " + std::to_string(y) + 
                "   w: " + std::to_string(w) + 
                "   h: " + std::to_string(h);
    }
    std::string to_string_min_max() const {
        return "min_x: " + std::to_string(min_x()) + 
                "   min_y: " + std::to_string(min_y()) + 
                "   max_x: " + std::to_string(max_x()) + 
                "   max_y: " + std::to_string(max_y());
    }
    bool operator==(const RectT<T> other) const {
        return 
            x == other.x &&
            y == other.y &&
            w == other.w &&
            h == other.h;
    }
    bool operator!=(const RectT<T> other) const {
        return !(*this == other);
    }
};
typedef RectT<double> Rect;


void _write_to_vector(void *vector_ptr, void *data, int size) {
    std::vector<unsigned char>* vec = (std::vector<unsigned char>*)vector_ptr;
    //PrintExpr(vicmil::to_binary_str(vec));
    vec->resize(size);
    memcpy(&(*vec)[0], data, size);
}


struct ImageRGBA_UChar {
    int w;
    int h;
    std::vector<ColorRGBA_UChar> pixels;
    void resize(unsigned int new_width, unsigned int new_height) {
        // Note! Will not preserve content of image. Only the pixel vector will be resized
        w = new_width;
        h = new_height;
        pixels.resize(new_width * new_height);
    }
    void copy_to_image(ImageRGBA_UChar* other_image, int x, int y) {
        // Copy this image to other image, so that this image start corner is at x, y of other image
        int copy_max_x = std::min(other_image->w - x, w);
        int copy_max_y = std::min(other_image->h - y, h);
        for(int x2 = 0; x2 < copy_max_x; x2++) {
            for(int y2 = 0; y2 < copy_max_y; y2++) {
                *other_image->get_pixel(x2 + x, y2 + y) = *get_pixel(x2, y2);
            }
        }
    }
    ColorRGBA_UChar* get_pixel(int x, int y) {
        return &pixels[y * w + x];
    }
    unsigned char* get_pixel_data() {
        return (unsigned char*)((void*)(&pixels[0]));
    }
    const unsigned char* get_pixel_data_const() const {
        return (const unsigned char*)((void*)(&pixels[0]));
    }
    void set_pixel_data(unsigned char* data, int byte_count) {
        assert(byte_count == pixels.size() * sizeof(ColorRGBA_UChar));
        std::memcpy(&pixels[0], data, byte_count);
    }

    static ImageRGBA_UChar load_png_from_file(std::string filename) {
        // If it failed to load the file, the width will be zero for the returned image object
        ImageRGBA_UChar return_image = ImageRGBA_UChar();
        int w = 0;
        int h = 0;
        int n = 0;
        int comp = 4; // r, g, b, a
        unsigned char *data = stbi_load(filename.c_str(), &w, &h, &n, comp);
        return_image.resize(w, h);
        if(w != 0) {
            return_image.set_pixel_data(data, w * h * 4);
        }
        stbi_image_free(data);
        return return_image;
    }
    void save_as_png(std::string filename) const {
        int comp = 4; // r, g, b, a
        const void *data = get_pixel_data_const();
        int stride_in_bytes = 0;
        stbi_write_png(filename.c_str(), w, h, comp, data, stride_in_bytes);
    }

    std::vector<unsigned char> to_png_as_bytes() const {
        int comp = 4; // r, g, b, a
        const void* data = &pixels[0];
        int stride_in_bytes = 0;
        stbi_write_func& write_func = _write_to_vector;

        std::vector<unsigned char> vec = std::vector<unsigned char>();
        std::vector<unsigned char>* vec_ptr = &vec;
        stbi_write_png_to_func(write_func, vec_ptr, w, h, comp, data, stride_in_bytes);
        return vec;
    }
    static ImageRGBA_UChar png_as_bytes_to_image(const unsigned char* bytes, int length) {
        int w;
        int h;
        int n;
        int comp = 4; // r, g, b, a

        ImageRGBA_UChar return_image = ImageRGBA_UChar();

        unsigned char *data = stbi_load_from_memory(bytes, length, &w, &h, &n, comp);
        return_image.resize(w, h);
        return_image.set_pixel_data(data, w * h * 4);
        stbi_image_free(data);
        return return_image;
    }
    static ImageRGBA_UChar png_as_bytes_to_image(const std::vector<unsigned char>& data) {
        return png_as_bytes_to_image(&data[0], data.size());
    }
    void flip_vertical() {
        for(int x = 0; x < w; x++) {
            for(int y = 0; y < h/2; y++) {
                ColorRGBA_UChar tmp = *get_pixel(x, y);
                *get_pixel(x, y) = *get_pixel(x, h - y - 1);
                *get_pixel(x, h - y - 1) = tmp;
            }
        }
    }
    // Set the entire image to the selected color
    void fill(ColorRGBA_UChar new_color) {
        for(int i = 0; i < pixels.size(); i++) {
            pixels[i] = new_color;
        }
    }
};

// Image of floats, typically in the range [0,1], can be used to store depth images for example
struct Image_float {
    int w;
    int h;
    std::vector<float> pixels;
    void resize(unsigned int new_width, unsigned int new_height) {
        pixels.resize(new_width * new_height);
    }
    float* get_pixel(int x, int y) {
        return &pixels[y * w + x];
    }
    float* get_pixel_data() {
        return &pixels[0];
    }
    const float* get_pixel_data_const() const {
        return &pixels[0];
    }
    void set_pixel_data(float* data, int byte_count) {
        assert(byte_count == pixels.size() * sizeof(float));
        std::memcpy(&pixels[0], data, byte_count);
    }
    void flip_vertical() {
        for(int x = 0; x < w; x++) {
            for(int y = 0; y < h/2; y++) {
                float tmp = *get_pixel(x, y);
                *get_pixel(x, y) = *get_pixel(x, h - y - 1);
                *get_pixel(x, h - y - 1) = tmp;
            }
        }
    }
    ImageRGBA_UChar to_image_rgba_uchar() {
        ImageRGBA_UChar new_image;
        // Assume float values are in the range 0 and 1
        // make interval between green and red, where red is 0 and green is 1
        new_image.resize(w, h);
        for(int i = 0; i < pixels.size(); i++) {
            float pixel_value = std::max(std::min(pixels[i], 1.0f), 0.0f); // Clip it in range 0 and 1
            new_image.pixels[i] = ColorRGBA_UChar((1.0-pixel_value)*255, pixel_value*255, 0, 1);
        }
        return new_image;
    }
};

// ============================================================
//                           Loading fonts
// ============================================================

// For loading true type fonts (.ttf, and .otf fonts)
struct FontLoader {
    stbtt_fontinfo info;
    std::vector<unsigned char> font_data;

    // Calculated from line height
    int line_height;
    float scale;
    int ascent; // Line spacing in y direction
    int descent;
    int lineGap;

    void load_font_from_memory(unsigned char* fontBuffer_, int size, int line_height_=64) {
        // Load font into buffer
        font_data.resize(size);
        memcpy(&font_data[0], fontBuffer_, size);

        // Prepare font
        if (!stbtt_InitFont(&info, &font_data[0], 0))
        {
            printf("failed\n");
        }
        set_line_height(line_height_); // Set default line height
    }
    void load_font_from_file(std::string filepath, int line_height_=64) {
        vicmil::FileManager file = vicmil::FileManager(filepath);
        std::vector<char> data = file.read_entire_file();
        load_font_from_memory((unsigned char*)&data[0], data.size(), line_height_);
    }

    void set_line_height(int new_line_height) {
        line_height = new_line_height;
        scale = stbtt_ScaleForPixelHeight(&info, line_height);
        int ascent; // Line spacing in y direction
        int descent;
        int lineGap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
        ascent = roundf(ascent * scale);
        descent = roundf(descent * scale);
    }

    void _get_character_advancement(const int character, int* advanceWidth, int* leftSideBearing) {
        // Advance width is how much to advance to the right
        // leftSideBearing means that it overlaps a little with the previous character
        stbtt_GetCodepointHMetrics(&info, character, advanceWidth, leftSideBearing);
        *advanceWidth = roundf(*advanceWidth * scale);
        *leftSideBearing = roundf(*leftSideBearing * scale);
    }

    int _get_kernal_advancement(const int character1, const int character2) {
        int kern = stbtt_GetCodepointKernAdvance(&info, character1, character2);
        return roundf(kern * scale);
    }

    RectT<int> _get_character_bounding_box(const int character) {
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&info, character, &ax, &lsb);

        // Get bounding box for character (may be offset to account for chars that dip above or below the line)
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, character, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
        return RectT<int>(c_x1, c_y1, c_x2 - c_x1, c_y2 - c_y1);
    }

    // Get image of character
    ImageRGBA_UChar get_character_image_rgba(const int character, ColorRGBA_UChar color_mask=ColorRGBA_UChar(255, 255, 255, 255)) {
        RectT<int> bounding_box = _get_character_bounding_box(character);
        ImageRGBA_UChar return_image = ImageRGBA_UChar();
        std::vector<unsigned char> pixels_char_vec = std::vector<unsigned char>();
        pixels_char_vec.resize(bounding_box.w * bounding_box.h);
        stbtt_MakeCodepointBitmap(&info, (unsigned char*)&pixels_char_vec[0], bounding_box.w, bounding_box.h, bounding_box.w, scale, scale, character);
        return_image.resize(bounding_box.w, bounding_box.h);
        for(int i = 0; i < pixels_char_vec.size(); i++) {
            return_image.pixels[i] = ColorRGBA_UChar(color_mask.r, color_mask.g, color_mask.b, pixels_char_vec[i] * color_mask.a / 255.0);
        }
        return return_image;
    }

    // Get where font images in a text should be placed. 
    // Some fonts may take into consideration which letters are next to each other, so-called font kerning
    // Characters are specified in unicode(but normal ascii will be treated as usual)
    std::vector<RectT<int>> get_character_image_positions(const std::vector<int> characters) {
        std::vector<RectT<int>> return_vec = {};
        return_vec.reserve(characters.size());

        int x = 0;
        int y = 0;
        for(int i = 0; i < characters.size(); i++) {
            // Determine if we should go to a new line
            if(characters[i] == '\n') {
                return_vec.push_back(RectT<int>(0, 0, 0, 0));
                y += line_height;
                x = 0;
                continue;
            }

            // Get bounding box for character
            RectT<int> image_pos = _get_character_bounding_box(characters[i]);
            image_pos.x += x;
            image_pos.y += ascent + y;

            int advanceWidth;
            int leftSideBearing;
            _get_character_advancement(characters[i], &advanceWidth, &leftSideBearing);
            image_pos.x += leftSideBearing;

            // Push back bounding box
            return_vec.push_back(image_pos);

            // Increment position if there is another letter after
            if(i + 1 != characters.size()) {
                x += advanceWidth;
                x += _get_kernal_advancement(characters[i], characters[i + 1]);
            }
        }
        return return_vec;
    }

    // Get the glyph index of character
    // (Can be used to determine if two letters correspond to the same font image)
    int get_glyph_index(const int character) {
        int glyphIndex = stbtt_FindGlyphIndex(&info, character);
        //if (glyphIndex == 0) {
        //    Print("Glyph not found for codepoint: " << character);
        //}
        return glyphIndex;
    }
    // Determine if a letter/character/unicode character is a part of the loaded font
    bool character_is_part_of_font(const int character) {
        return get_glyph_index(character) != 0;
    }
};

/**
 * Supports loading multiple .ttf or .otf fonts at the same time
 */
struct MultiFontLoader {
    std::vector<FontLoader> fontLoaders;  // Stores multiple fonts
    int line_height = 64;

    // Load a font from memory
    void load_font_from_memory(unsigned char* fontBuffer, int size) {
        fontLoaders.push_back(FontLoader());
        fontLoaders.back().load_font_from_memory(fontBuffer, size, line_height);
    }

    // Load a font from a file
    void load_font_from_file(const std::string& filepath) {
        fontLoaders.push_back(FontLoader());
        fontLoaders.back().load_font_from_file(filepath, line_height);
    }

    // Set line height for all loaded fonts
    void set_line_height(int line_height_) {
        line_height = line_height_;
        for (auto& loader : fontLoaders) {
            loader.set_line_height(line_height_);
        }
    }

    // Find the first font that supports a given character
    FontLoader* find_font_with_character(int character) {
        for (auto& loader : fontLoaders) {
            if (loader.character_is_part_of_font(character)) {
                return &loader;
            }
        }
        return nullptr;  // No font supports the character
    }

    // Get character image with fallback mechanism
    ImageRGBA_UChar get_character_image_rgba(int character, ColorRGBA_UChar color_mask = ColorRGBA_UChar(255, 255, 255, 255)) {
        FontLoader* fontLoader = find_font_with_character(character);
        if (!fontLoader) return ImageRGBA_UChar();  // Return empty image if no font supports the character
        return fontLoader->get_character_image_rgba(character, color_mask);
    }

    std::vector<RectT<int>> get_character_image_positions(const std::vector<int>& characters) {
        std::vector<RectT<int>> positions;
        positions.reserve(characters.size());
    
        int x = 0;  // Track x-position for character placement
        int y = 0;
        int ascent = 0;  // Store max ascent among used fonts for alignment
    
        // Determine ascent (highest baseline for proper alignment)
        for (int c : characters) {
            FontLoader* font = find_font_with_character(c);
            if (font && font->ascent > ascent) {
                ascent = font->ascent;
            }
        }
    
        // Iterate over characters to compute their positions
        for (size_t i = 0; i < characters.size(); i++) {
            // Determine if we should go to a new line
            if(characters[i] == '\n') {
                positions.push_back(RectT<int>(x, 0, 0, 0));
                y += line_height;
                x = 0;
                continue;
            }

            int character = characters[i];
    
            FontLoader* font = find_font_with_character(character);
            if (!font) continue;  // Skip if no valid font found
    
            // Get character bounding box
            RectT<int> image_pos = font->_get_character_bounding_box(character);
            image_pos.x += x;  // Adjust x position
            image_pos.y += ascent + y;  // Align to max ascent
    
            int advanceWidth, leftSideBearing;
            font->_get_character_advancement(character, &advanceWidth, &leftSideBearing);
            image_pos.x += leftSideBearing;  // Adjust by left side bearing
    
            // Store bounding box position
            positions.push_back(image_pos);
    
            // Increment position for next character
            if (i + 1 < characters.size()) {
                int kern = font->_get_kernal_advancement(character, characters[i + 1]);
                x += advanceWidth + kern;
            }
        }
    
        return positions;
    }

    // Check if a character is supported by any loaded font
    bool character_is_part_of_font(int character) {
        return find_font_with_character(character) != nullptr;
    }
};
}