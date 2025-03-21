#include "util_std.hpp"
#include "util_obj_loader.hpp"
#include "util_opengl.hpp"

vicmil::Window window;
vicmil::DefaultGpuPrograms gpu_programs;
std::vector<vicmil::VertexCoordColor> vertices;

vicmil::TextInput text_input;

void update() {
    std::vector<SDL_Event> events = vicmil::update_SDL();
    if(text_input.update(events)) {
        Print("Text: " << text_input.getInputTextUTF8WithCursor());
        Print("Composition text: " << text_input.getCompositionTextUTF8());
    }

    vicmil::clear_screen();
    gpu_programs.draw_2d_VertexCoordColor_vertex_buffer(vertices);
    window.show_on_screen();
}


void init() {
    vicmil::init_SDL();
    window = vicmil::Window(512, 512);

    gpu_programs = vicmil::DefaultGpuPrograms();
    gpu_programs.init_default_gpu_programs();

    vicmil::add_color_rect_to_triangle_buffer(vertices, vicmil::GuiEngine::RectGL(-0.5, 0.5, 1.0, 1.0), 1, 255, 0, 0, 255);

    text_input = vicmil::TextInput();
    text_input.start();
}

int main() {
    vicmil::set_app_init(init);
    vicmil::set_app_update(update);
    vicmil::app_start();
    return 0;
}