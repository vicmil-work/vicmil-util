#include "util_std.hpp"
#include "util_obj_loader.hpp"
#include "util_opengl.hpp"
#include "noto_sans_mono_jp.hpp"
#include "noto_sans_mono.hpp"

vicmil::Window window;
vicmil::DefaultGpuPrograms gpu_programs;
std::vector<vicmil::VertexTextureCoord> vertices;

vicmil::TextInput text_input;
vicmil::ImageTextureManager* texture_manager;
vicmil::MultiFontLoader font_loader;
vicmil::GuiEngine gui_engine;

int window_w = 800;
int window_h = 512;

// Returns true if the unicode did not exist before
bool add_unicode(int unicode_char) {
    std::string char_label = vicmil::ImageTextureManager::get_unicode_label(unicode_char);
    if(texture_manager->contains_image(char_label)) {
        return false;
    }

    vicmil::ImageRGBA_UChar char_image = font_loader.get_character_image_rgba(unicode_char);

    if(texture_manager->add_image(char_label, char_image)) {
        return true;
    }
    return false;
}

void add_cursor_image() {
    std::string char_label = "cursor";
    if(texture_manager->contains_image(char_label)) {
        return;
    }

    vicmil::ImageRGBA_UChar char_image = vicmil::ImageRGBA_UChar();
    char_image.resize(2, 64);
    char_image.fill(vicmil::ColorRGBA_UChar(255, 255, 255, 255));

    texture_manager->add_image(char_label, char_image);
}

void update() {
    std::vector<SDL_Event> events = vicmil::update_SDL();
    if(text_input.update(events)) {
        Print("Text: " << text_input.getInputTextUTF8WithCursor());
        Print("Composition text: " << text_input.getCompositionTextUTF8());
    }

    std::vector<int> character_unicodes = vicmil::utf8ToUnicodeCodePoints(text_input.getInputTextUTF8());

    bool updated_unicode = false;
    for(int character_unicode: character_unicodes) {
        if(add_unicode(character_unicode)) {
            updated_unicode = true;
        }
    }
    add_cursor_image(); // Add the cursor too

    if(updated_unicode) {
        texture_manager->update_gpu_texture();
    }

    std::vector<vicmil::RectT<int>> character_image_positions = font_loader.get_character_image_positions(character_unicodes);
    vertices = {};
    for(int i = 0; i < character_unicodes.size(); i++) {
        vicmil::RectT<int> char_pos = character_image_positions[i];
        vicmil::GuiEngine::RectGL screen_pos = gui_engine.rect_to_rect_gl(vicmil::GuiEngine::Rect(char_pos.x, char_pos.y+100, char_pos.w, char_pos.h));
        vicmil::GuiEngine::RectGL texture_pos = texture_manager->get_image_pos_gl(vicmil::ImageTextureManager::get_unicode_label(character_unicodes[i]));
        vicmil::add_texture_rect_to_triangle_buffer(vertices, screen_pos, 1, texture_pos);
    }

    // Draw the cursor too
    vicmil::RectT<int> cursor_pos = vicmil::RectT<int>(0, -55, 2, 64);
    if(text_input.cursorPos > 0) {
        cursor_pos.x += character_image_positions[text_input.cursorPos-1].x + character_image_positions[text_input.cursorPos-1].w;
        for(int i = 0; i < text_input.cursorPos-1; i++) {
            if(character_unicodes[i] == '\n') {
                cursor_pos.y += 64;
            }
        }
    }
    vicmil::GuiEngine::RectGL screen_pos = gui_engine.rect_to_rect_gl(vicmil::GuiEngine::Rect(cursor_pos.x, cursor_pos.y+100, cursor_pos.w, cursor_pos.h));
    vicmil::GuiEngine::RectGL texture_pos = texture_manager->get_image_pos_gl("cursor");
    vicmil::add_texture_rect_to_triangle_buffer(vertices, screen_pos, 2, texture_pos);

    vicmil::clear_screen();
    gpu_programs.draw_2d_VertexTextureCoord_vertex_buffer(vertices, texture_manager->gpu_texture);
    window.show_on_screen();
}


void init() {
    vicmil::init_SDL();
    window = vicmil::Window(window_w, window_h);

    gpu_programs = vicmil::DefaultGpuPrograms();
    gpu_programs.init_default_gpu_programs();

    text_input = vicmil::TextInput();
    text_input.inputText = vicmil::utf8ToUnicodeCodePoints("猫は窓の外を\nじっと見つめている。 \n- The cat is staring \noutside the window.");
    text_input.start();

    font_loader.load_font_from_memory(NOTO_SANS_MONO_HPP_data, NOTO_SANS_MONO_HPP_size);
    font_loader.load_font_from_memory(NOTO_SANS_MONO_JP_HPP_data, NOTO_SANS_MONO_JP_HPP_size);
    texture_manager = new vicmil::ImageTextureManager(512, 512);
    texture_manager->update_gpu_texture();
    gui_engine.set_screen_size(window_w, window_h);
}

int main() {
    vicmil::set_app_init(init);
    vicmil::set_app_update(update);
    vicmil::app_start();
    return 0;
}