#include "util_std.hpp"
#include "util_obj_loader.hpp"
#include "util_opengl.hpp"

vicmil::Window window;
vicmil::DefaultGpuPrograms gpu_programs;
std::vector<vicmil::VertexCoordColor> vertices;

void update() {
    vicmil::update_SDL();
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
}

int main() {
    vicmil::set_app_init(init);
    vicmil::set_app_update(update);
    vicmil::app_start();
    return 0;
}