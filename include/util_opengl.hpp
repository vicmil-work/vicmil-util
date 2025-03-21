
// ============================================================
//                           Include
// ============================================================

#pragma once

#include "glm/glm/gtx/transform.hpp"
#include "glm/glm/gtx/string_cast.hpp"
#include "util_std.hpp"
#include "util_stb.hpp"
#include "util_obj_loader.hpp"

#if defined(__EMSCRIPTEN__)
    #include <SDL.h>
    #include <SDL_opengles2.h>
#elif defined(__MINGW64__) 
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_image.h>

    // Glew is required to make it work on windows with mingw
    #include <gl\glew.h>
    #include <SDL2/SDL_opengl.h>
#else // If on linux
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_image.h>
    #include <GL/glew.h>
    #include <SDL2/SDL_opengl.h>
#endif

//#define GLM_FORCE_UNRESTRICTED_GENTYPE
//#include "../deps/glm/glm/gtc/matrix_transform.hpp"
//#include "../deps/glm/glm/gtx/transform.hpp"
//#include "../deps/glm/glm/ext/matrix_transform.hpp" // perspective, translate, rotate

// ============================================================
//                       Error handling
// ============================================================

namespace vicmil {
/**
 * Clear out any OpenGL errors and print out the errors if there was any
 *  Returns -1 if any errors were detected
*/
static int GLClearErrors() {
    bool error_detected = false;
    int iter = 0;
    while(GLenum error = glGetError()) {
        Print("[OpenGl Error] \"" << error << "\"");
        error_detected = true;
        iter += 1;
        if(iter > 10) {
            break;
        }
    }
    if(error_detected) {
        Print("Error detected")
        return -1;
    }
    return 0;
}

/**
 * Detect OpenGL errors both before and after the expression was executed
*/
#define GLCall(x) \
if(vicmil::GLClearErrors() == -1) {ThrowError("Unhandled opengl error before call!");} \
x; \
if(vicmil::GLClearErrors() == -1) {ThrowError("Opengl call caused error!");}


// ============================================================
//                       GPU Programming
// ============================================================

/**
 * Convert the shader type as int to a string
*/
std::string get_shader_type_name(unsigned int shader_type) {
    if(shader_type == GL_FRAGMENT_SHADER) {
        return "fragment shader";
    }
    if(shader_type == GL_VERTEX_SHADER) {
        return "vertex shader";
    }
    return "unknown shader";
}

/**
 * Wrapper class to create and manipulate a shader on the GPU
 *   A shader is basically a program that runs on the GPU
 *   The vertex shader is executed once for every triangle corner
 *   The fragment shader is executed once for every pixel in every triangle, 
 *      it will interpolate between the triangle corners as input data
 * 
 *   You will need both a fragment shader AND a vertex shader to see anything on the screen
*/   
class Shader {
public:
    std::string raw_content;  // The source code of the shader
    unsigned int id;  // The id of the shader, used to reference it on the GPU
    unsigned int type; // The type of the shader, eg fragment shader or vertex shader
    Shader() {}

    /**
     * Create a new shader on the GPU
     * @arg str_: the shader program
     * @arg type_; The type of shader as an int, eg GL_FRAGMENT_SHADER or GL_VERTEX_SHADER
    */
    static Shader from_str(const std::string& str_, unsigned int type_) {
        Shader new_shader = Shader();
        new_shader.raw_content = str_;
        new_shader.compile_shader(type_);
        return new_shader;
    }
    /**
     * Compile the shader using the source code, this will generate a new shader instance on the GPU
    */
    void compile_shader(unsigned int shader_type) {
        type = shader_type;
        id = glCreateShader(shader_type);
        const char* ptr = raw_content.c_str();
        glShaderSource(id, 1, &ptr, nullptr);
        glCompileShader(id);
        print_errors("Failed to compile");
    }
    void print_errors(std::string context_message) {
        int result;
        GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
        if(result == GL_FALSE) {
            int length;
            GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
            char* err_message = (char*) alloca(length * sizeof(char));
            GLCall(glGetShaderInfoLog(id, length, &length, err_message));
            std::cout << context_message << ": " << get_shader_type_name(type) << std::endl;
            std::cout << err_message << std::endl;
            GLCall(glDeleteShader(id));
        }
    }

};

/**
 * Wrapper class for vertex shader
*/
class VertexShader {
public:
    Shader shader;
    static VertexShader from_str(const std::string& str_) {
        VertexShader new_shader = VertexShader();
        new_shader.shader = Shader::from_str(str_, GL_VERTEX_SHADER);
        return new_shader;
    }
    VertexShader() {}
};

/**
 * Wrapper class for fragment shader
*/
class FragmentShader {
public:
    Shader shader;
    static FragmentShader from_str(const std::string& str_) {
        FragmentShader new_shader = FragmentShader();
        new_shader.shader = Shader::from_str(str_, GL_FRAGMENT_SHADER);
        return new_shader;
    }
    FragmentShader() {}
};

/**
 * Wrapper class for a GPU program
 * A GPU program consist of two sub-programs, the vertex shader and the fragment shader
*/
class GPUProgram {
public:
    unsigned int id = 0; // A reference to the program on the GPU, 0 means the program has not been initialized
    static GPUProgram from_strings(const std::string& vert_str, const std::string& frag_str) {
        GPUProgram new_program = GPUProgram();
        new_program.id = glCreateProgram();
        VertexShader vert_shader = VertexShader::from_str(vert_str);
        FragmentShader frag_shader = FragmentShader::from_str(frag_str);
        new_program._attach_shaders(vert_shader, frag_shader);
        new_program._delete_shaders(vert_shader, frag_shader); // The shaders are already linked and are therefore not needed
        return new_program;
    }
    /**
     * This tells OpenGL that we will use this program going forward, until we bind another program
     *   Only one program can be bound at the same time
    */
    void bind_program() {
        GLCall(glUseProgram(id));
    }
    /**
     * Delete both the fragment shader and the vertex shader from the GPU
    */
    void _delete_shaders(VertexShader& vert_shader, FragmentShader& frag_shader) {
        GLCall(glDeleteShader(vert_shader.shader.id));
        GLCall(glDeleteShader(frag_shader.shader.id));
    }
    /**
     * Attach the shaders to the current program
     *  this tells the GPU that these shaders are the ones that make up the program
     *  The vert_shader and frag_shader are copied to the program, so we don't need 
     *  to keep the old shaders for the program to work
    */
    void _attach_shaders(VertexShader& vert_shader, FragmentShader& frag_shader) {
        GLCall(glAttachShader(id, vert_shader.shader.id));
        GLCall(glAttachShader(id, frag_shader.shader.id));
        GLCall(glLinkProgram(id));
        GLCall(glValidateProgram(id));
    }
    /**
     * Delete the program from the GPU
    */
    void delete_program() {
        glDeleteProgram(id);
    }
    ~GPUProgram() {}
};


// ============================================================
//                       GPU Buffers
// ============================================================

class GLBuffer {
public:
    unsigned int buffer_id;
    unsigned int type;
    unsigned int size;
    bool dynamic_buffer;
    GLBuffer() {}
    static GLBuffer generate_buffer(unsigned int size_in_bytes, const void* data, unsigned int buffer_type, bool dynamic_buffer_=false) {
        GLBuffer new_buffer = GLBuffer();
        new_buffer.type = buffer_type;
        new_buffer.size = size_in_bytes;
        new_buffer.dynamic_buffer = dynamic_buffer_;
        GLCall(glGenBuffers(1, &new_buffer.buffer_id)); // Create the buffer
        new_buffer.bind_buffer();
        if(dynamic_buffer_) {
            GLCall(glBufferData(buffer_type, size_in_bytes, data, GL_DYNAMIC_DRAW)); // Set data in buffer
        }
        else {
            GLCall(glBufferData(buffer_type, size_in_bytes, data, GL_STATIC_DRAW)); // Set data in buffer
        }
        return new_buffer;
    }
    void overwrite_buffer_data(unsigned int size_in_bytes, const void* data) {
        bind_buffer();
        size = size_in_bytes;
        if(dynamic_buffer) {
            GLCall(glBufferData(type, size_in_bytes, data, GL_DYNAMIC_DRAW)); // Set data in buffer
        }
        else {
            GLCall(glBufferData(type, size_in_bytes, data, GL_STATIC_DRAW)); // Set data in buffer
        }
    }
    void substitute_buffer_data(unsigned int size_in_bytes, const void* data, unsigned int offset=0) {
        GLCall(glBufferSubData(type, offset, size_in_bytes, data));
    }
    void bind_buffer() {
        // You must first bind a buffer in order to use or modify it, only one buffer can be binded at the same time
        glBindBuffer(type, buffer_id);
    }
    void delete_buffer() {
        glDeleteBuffers(1, &buffer_id);
    }
    unsigned int number_of_floats() {
        return size / sizeof(float);
    }
    unsigned int number_of_unsigned_ints() {
        return size / sizeof(unsigned int);
    }
};

class GLIndexBuffer {
public:
    GLBuffer buffer;
    GLIndexBuffer() {}
    static GLIndexBuffer create_buffer(std::vector<unsigned int> data) {
        GLIndexBuffer new_buffer = GLIndexBuffer();
        new_buffer.buffer = GLBuffer::generate_buffer(sizeof(unsigned int) * data.size(), &data[0], GL_ELEMENT_ARRAY_BUFFER);
        return new_buffer;
    }
};

class GLVertexBuffer {
public:
    GLBuffer buffer;
    GLVertexBuffer() {}
    static GLVertexBuffer create_buffer(std::vector<float> data) {
        GLVertexBuffer new_buffer = GLVertexBuffer();
        new_buffer.buffer = GLBuffer::generate_buffer(sizeof(float) * data.size(), &data[0], GL_ARRAY_BUFFER);
        return new_buffer;
    }
};


class VertexBufferElement {
public:
    unsigned int type;
    unsigned int count;
    unsigned char normalized = GL_FALSE;
    std::string attrib_name = ""; // The name of the parameter in the compiled code, for example "position"
    VertexBufferElement() {}
    VertexBufferElement(unsigned int type_, unsigned int count_, std::string attrib_name_, unsigned char normalized_ = GL_FALSE) {
        type = type_;
        count = count_;
        attrib_name = attrib_name_;
        normalized = normalized_;
    }

    unsigned int get_size() {
        if(type == GL_FLOAT) {
            return sizeof(float) * count;
        }
        if(type == GL_UNSIGNED_INT) {
            return 2 * count;
        }
        if(type == GL_INT) {
            return 4 * count;
        }
        if(type == GL_UNSIGNED_BYTE) {
            return 1 * count;
        }
        Print("Unknown type");
        return 4 * count;
    }
};

/**
 * Help class specify how the vertex data are aligned in the memory
*/
class VertexBufferLayout {
public:
    std::vector<VertexBufferElement> buffer_elements;
    VertexBufferLayout() {}
    VertexBufferLayout(std::vector<VertexBufferElement> buffer_elements_) {
        buffer_elements = buffer_elements_;
    }
    unsigned int get_size() {
        int size_ = 0;
        for(unsigned int i = 0; i < buffer_elements.size(); i++) {
            size_ += buffer_elements[i].get_size();
        }
        return size_;
    }
    void set_vertex_buffer_layout(GPUProgram gpu_program) {
        if(gpu_program.id == 0) {
            ThrowError("Program has not been initialized");
        }
        unsigned int offset = 0; // Offset is between each element
        unsigned int stride = this->get_size(); // Stride is between a set of all elements
        for(unsigned int i = 0; i < buffer_elements.size(); i++) {
            unsigned int posAttrib = glGetAttribLocation(gpu_program.id, buffer_elements[i].attrib_name.c_str());
            GLCall(glEnableVertexAttribArray(posAttrib));
            GLCall(glVertexAttribPointer(
                posAttrib, 
                buffer_elements[i].count, 
                buffer_elements[i].type, 
                buffer_elements[i].normalized,
                stride,
                reinterpret_cast<const void*>(offset)
            ));
            offset += buffer_elements[i].get_size();
        }
    }
};

/**
 * It is often good to group an index buffer and a vertex buffer as a pair
 *  - A vertex buffer contains all the triangle corners in a jumbled mess
 *  - The Index buffer chooses aming all the corners to decide which corners should make up a triangle
*/
class IndexVertexBufferPair {
public:
    GLBuffer vertex_buffer;
    GLBuffer index_buffer;
    static IndexVertexBufferPair from_raw_data(void* index_data, unsigned int index_data_byte_size, void* vertex_data, unsigned int vertex_data_byte_size) {
        IndexVertexBufferPair new_buffer_pair;
        new_buffer_pair.vertex_buffer = GLBuffer::generate_buffer(vertex_data_byte_size, vertex_data, GL_ARRAY_BUFFER);
        new_buffer_pair.index_buffer = GLBuffer::generate_buffer(index_data_byte_size, index_data, GL_ELEMENT_ARRAY_BUFFER);
        return new_buffer_pair;
    }
    /**
     * Overwrite all the data in the vertex and index buffer
    */
    void overwrite_data(void* index_data, unsigned int index_data_byte_size, void* vertex_data, unsigned int vertex_data_byte_size) {
        vertex_buffer.overwrite_buffer_data(vertex_data_byte_size, vertex_data);
        index_buffer.overwrite_buffer_data(index_data_byte_size, index_data);
    }
    /**
     * Bind the vertex and index buffers on the GPU. This means that it 
     *  is these buffers the GPU refers to until another buffer calls bind()
     *  there can only exist one bound buffer of each type at one time
    */
    void bind() {
        vertex_buffer.bind_buffer();
        index_buffer.bind_buffer();
    }
    /**
     * Use buffer to perform program on GPU, the GPU Program must be bound before this call
     * @arg triangle_count: The number of triangles to use, if it is -1, use all triangles in index buffer
     * @arg offset_in_bytes: The offset of where to begin in index buffer, great if you only want to use
     *    a part of the buffer, otherwise just leave it
    */
    void draw(int triangle_count=-1, unsigned int offset_in_bytes=0) {
        if(triangle_count > 0) {
            GLCall(glDrawElements(GL_TRIANGLES, triangle_count * 3, GL_UNSIGNED_INT, reinterpret_cast<const void*>(offset_in_bytes)));
        }
        else {
            //Debug("index buffer size: " << index_buffer.number_of_unsigned_ints());
            GLCall(glDrawElements(GL_TRIANGLES, index_buffer.number_of_unsigned_ints(), GL_UNSIGNED_INT, nullptr));
        }
    }

    void delete_buffer_pair() {
        vertex_buffer.delete_buffer();
        index_buffer.delete_buffer();
    }
};

/**
 * The index buffer is made up of a list of triangles, this can be one of them
 *   The indicies refer to the vertex buffer
*/
struct IndexedTriangleI3 {
    int index1; // index of triangle corners
    int index2; // index of triangle corners
    int index3; // index of triangle corners
    IndexedTriangleI3() {}
    IndexedTriangleI3(unsigned int i1, unsigned int i2, unsigned int i3) {
        index1 = i1;
        index2 = i2;
        index3 = i3;
    }
    static IndexedTriangleI3 from_str(std::string i1, std::string i2, std::string i3) {
        return IndexedTriangleI3(std::stof(i1), std::stof(i2), std::stof(i3));
    }
    /**
     * If we assume that the triangles are laid out linearly in index buffer, eg just one triangle after the other
     *   Then we can just get the indecies very simply by:
    */
    static std::vector<IndexedTriangleI3> linear_indexed_triangles(int triangle_count) {
        std::vector<IndexedTriangleI3> vec;
        vec.resize(triangle_count);
        for(int i = 0; i < triangle_count; i++) {
            vec[i].index1 = i*3 + 0;
            vec[i].index2 = i*3 + 1;
            vec[i].index3 = i*3 + 2;
        }
        return vec;
    }
};

// Assume all the data is layed out nicely, eg (triangle_corner 1, 2, 3), (triangle_corner 1, 2, 3). etc.
// Each vertex refers to a triangle corner
class VertexBuffer {
public:
    GLBuffer vertex_buffer;
    int buffer_vertex_count = 0;
    static VertexBuffer from_raw_data(void* vertex_data, unsigned int vertex_data_byte_size, int vertex_count_) {
        VertexBuffer new_buffer;
        new_buffer.vertex_buffer = GLBuffer::generate_buffer(vertex_data_byte_size, vertex_data, GL_ARRAY_BUFFER);
        new_buffer.buffer_vertex_count = vertex_count_;
        return new_buffer;
    }
    template<class VERTEX>
    static VertexBuffer from_vertex_vector(std::vector<VERTEX>& vec) {
        return from_raw_data(&vec[0], vec.size() * sizeof(VERTEX), vec.size());
    }
    /**
     * Overwrite all the data in the vertex buffer
    */
    void overwrite_data(void* vertex_data, unsigned int vertex_data_byte_size, int vertex_count_) {
        buffer_vertex_count = vertex_count_;
        vertex_buffer.overwrite_buffer_data(vertex_data_byte_size, vertex_data);
    }
    template<class VERTEX>
    void overwrite_vertex_vector(std::vector<VERTEX>& vec) {
        return overwrite_data(&vec[0], vec.size() * sizeof(VERTEX), vec.size());
    }
    /**
     * Bind the vertex buffer on the GPU. This means that it 
     *  is these buffers the GPU refers to until another buffer calls bind()
     *  there can only exist one bound buffer of each type at one time
    */
    void bind() {
        vertex_buffer.bind_buffer();
    }
    /**
     * Use buffer to perform program on GPU, the GPU Program must be bound before this call
     * @arg triangle_count: The number of triangles to use
     * @arg first: The index of the first triangle to draw(skipps all triangles before it)
    */
    void draw_triangles(int triangle_count_ = -1, unsigned int first=0) {
        if(triangle_count_ == -1) {
            GLCall(glDrawArrays(GL_TRIANGLES, first, buffer_vertex_count));
        }
        else {
            GLCall(glDrawArrays(GL_TRIANGLES, first, triangle_count_ * 3));
        }
    }
    void draw_points(int point_count_ = -1, unsigned int first=0) {
        #ifndef __EMSCRIPTEN__ // Custom point size not supported in emscripten, defaults to 1.0
            glPointSize(1.0f);
        #endif
        if(point_count_ == -1) {
            GLCall(glDrawArrays(GL_POINTS, first, buffer_vertex_count));
        }
        else {
            GLCall(glDrawArrays(GL_POINTS, first, point_count_));
        }
    }
    void delete_buffer() {
        vertex_buffer.delete_buffer();
    }
};

// With no rotation, increasing an objects position in any of the three axes refers to the following: y is forward(into the screen), x is right, z is up
struct Perspective3DParameters {
    glm::vec3 rotation_center_xyz = {0, 0, 0}; // Around where everything should be rotated(typically around where the eye is)
    glm::vec3 eye_position_xyz = {0, 0, 0}; // From where everything will be seen from
    float zoom=glm::radians(60.0f); // How much zoomed in the camera should be
    float aspect_ratio=4.0f / 3.0f; // The aspect ratio of the window to render to

    // Everything further away or closer than the depth will not be drawn
    // Increasing the range will lead to worse precision within the range
    float min_depth = 0.1f; 
    float max_depth = 100.0f;

    void rotation_matrix_from_yaw_pitch(float yaw, float pitch) {
        glm::mat4 pitch_rotation = glm::rotate(pitch, glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 yaw_rotation = glm::rotate(yaw, glm::vec3(0.0, 0.0, 1.0));
        out_rotation_matrix = yaw_rotation * pitch_rotation;
    }
    void rotation_matrix_from_rotation_around_vector(glm::vec3 vec, float angle_rad) {
        out_rotation_matrix = glm::rotate(angle_rad, vec);
    }
    void rotation_from_relative_positions(glm::vec3 src_xyz, glm::vec3 look_at_xyz) {
        out_rotation_matrix = glm::mat4(1); // TODO
    }
    // Flip the axes, since opengl considers z to be backward, x to be left and y to be down
    // Usually needed at the end of all other transformations
    static glm::mat4 to_gl_axes() {
        // Lets make the perspective more intuitive
        // The positive z-axis should be straight up
        // The positive x-axis should be straight forward
        // The positive y-axis should be to the right
        glm::mat4 flipY = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
        glm::mat4 perspective_correction = glm::rotate(glm::radians(90.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::rotate(glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)) * flipY;
        return perspective_correction;
    }

    // Matricies that can be read
    glm::mat4 out_pos_translation;
    glm::mat4 out_offset_translation;
    glm::mat4 out_perspective;
    glm::mat4 out_perspective_correction;
    glm::mat4 out_rotation_matrix = glm::mat4(1);

    void _build() {
        out_pos_translation = glm::translate(rotation_center_xyz);
        out_offset_translation = glm::translate(eye_position_xyz - rotation_center_xyz);
        out_perspective = glm::perspective(
            zoom,
            aspect_ratio, 
            min_depth,         
            max_depth
        );
        out_perspective_correction = to_gl_axes();
    }

    glm::mat4 to_matrix() {
        // For the perspective(raw):
        // The negative y-axis is assumed to be pointing straight up
        // The negative z-axis will be straight forward(into the image)
        // The negative x-axis will be to the left
        _build(); // Update matricies
        // The right most matrix will be applied first, e.g. mat * vec
        glm::mat4 final_matrix = out_perspective * out_perspective_correction * out_offset_translation * out_rotation_matrix * out_pos_translation;
        return final_matrix;
    }
};

// Uniform buffer for handling rotations/perspective
class UniformBufferMat4f {
public:
    std::string attrib_name;
    UniformBufferMat4f(std::string attrib_name_="u_MVP") {
        attrib_name = attrib_name_;
    }
    void set_matrix(const glm::mat4 transform_mat, const GLuint program_id) {
        // Actually sets the matrix on the gpu
        int location = glGetUniformLocation(program_id, attrib_name.c_str());
        if (location == -1) {
            std::cout << "Uniform not found: " << attrib_name << std::endl;
            return;
        }
        const unsigned int matrix_count = 1;
        const char transpose_matrix = GL_FALSE;
        GLCall(glUniformMatrix4fv(location, matrix_count, transpose_matrix, &transform_mat[0][0]));
    }
    void set_rotation_vec(float angle, const glm::vec3 &v, GLuint program_id) {
        // Helper function to convert a rotation to a matrix
        glm::mat4 rotation = glm::rotate(angle, v);
        set_matrix(rotation, program_id);
    }
    void set_camera_perspective(Perspective3DParameters perspective_parameters, GLuint program_id) {
        set_matrix(perspective_parameters.to_matrix(), program_id);
    }
};

/*
void uniform_buffer_exmple() {
    gpu_program_3d.bind_program();
    static UniformBufferMat4f u_buffer = UniformBufferMat4f("u_MVP");
    static float angle = 0;
    angle += 0.01;
    u_buffer.set_rotation_vec(angle, glm::vec3(0.0, 1.0, 0.0), gpu_program_3d.id);
}
*/


// ============================================================
//                       GPU Texture
// ============================================================
/**
 * A texture lives on the GPU, and its data is not easily accessible
 * 
 * But with some OpenGL calls, data transfer is possible!
 * You can also render to a texture instead of the main window.
*/

class GPUTexture {
public:
    // The texture id, a reference to the GPU texture
    unsigned int renderedTexture = -1;

    // Parameter to specify whether a texture has been set
    //  if you try to bind no texture, do nothing
    bool no_texture = true;
    int _width = -1;
    int _height = -1;

    // [OPTIONAL] attached framebuffer is necessary if rendering to the texture
    GLuint FramebufferName = 0;
    GLuint depthRenderbuffer = 0;

    /** Create a new texture on the GPU from an RGBA image on the CPU
     * @param raw_image The image with pixel data to use
     * @return A reference to the texture on the GPU
    */
    static GPUTexture from_raw_image_rgba(ImageRGBA_UChar& raw_image) {
        return GPUTexture(raw_image.w, raw_image.h, raw_image.get_pixel_data());
    }

    GPUTexture() {}
    GPUTexture(unsigned int width, unsigned int height, unsigned char* pixel_data = nullptr) {
        Assert(vicmil::is_power_of_two(width) == true);
        Assert(vicmil::is_power_of_two(height) == true);
        _width = width;
        _height = height;
        no_texture = false;
        glGenTextures(1, &renderedTexture);

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, renderedTexture);

        // Generate the texture(with no predefined data)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);

        set_pixel_parameters_nearest(); // Set as default(if none set the image doesn't show)
    }

    /*
    Overwrite the image data stored on the texture on the gpu
    */
    void overwrite_texture_data(unsigned int width, unsigned int height, unsigned char* pixel_data = nullptr) {
        Assert(vicmil::is_power_of_two(width) == true);
        Assert(vicmil::is_power_of_two(height) == true);
        Assert(no_texture == false);

        glBindTexture(GL_TEXTURE_2D, renderedTexture);

        if (width != _width || height != _height) {
            // Reallocate texture if size has changed
            _width = width;
            _height = height;

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
        } else if (pixel_data) {
            // Just update the existing texture if size is the same
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
        }
    }
    void overwrite_texture_data(ImageRGBA_UChar& raw_image) {
        overwrite_texture_data(raw_image.w, raw_image.h, raw_image.get_pixel_data());
    }

    /**
     * Bind the texture so it is that texture that is referenced when drawing
     * @return None
    */
    void bind() {
        if(!no_texture) {
            glBindTexture(GL_TEXTURE_2D, renderedTexture);
        }
    }

    /**
     * Start rendering to this texture instead of the screen!
    */
    void start_render_to_texture(bool set_viewport = true) {
        if(!no_texture) {
            glGenFramebuffers(1, &FramebufferName);
            glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

            GLCall(glBindTexture(GL_TEXTURE_2D, renderedTexture));
            GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0));

            glGenRenderbuffers(1, &depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);

            // Allocate storage for the depth buffer, usually matching the size of the texture
            #ifndef __EMSCRIPTEN__
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
            #else
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _width, _height); // GL_DEPTH_COMPONENT not available using emscripten, have to specify size
            #endif

            // Attach the depth buffer to the framebuffer
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

            if(set_viewport) {
                // Set viewport(Where to render to)
                GLCall(glViewport(0, 0, _width, _height));
            }
        }
    }

    void stop_rendering_to_texture() {
        glDeleteFramebuffers(1, &FramebufferName);
        glDeleteRenderbuffers(1, &depthRenderbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind the default frame buffer
    }

    // Capture what is currently displayed on the screen
    // Note! Will be flipped upside down when returned(can be handled by just calling flip_vertical)
    static ImageRGBA_UChar get_screenshot(int out_texture_width, int out_texture_height) {
        ImageRGBA_UChar texture_image;
        texture_image.resize(out_texture_width, out_texture_height);

        // Get the texture data
        glReadPixels(0, 0, out_texture_width, out_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, texture_image.get_pixel_data());

        return texture_image;
    }

    // Capture the depth map of what is being displayed on the screen
    // Note! Will be flipped upside down when returned
    static Image_float get_depth_screenshot(int out_texture_width, int out_texture_height) {
        Image_float texture_image;
        texture_image.resize(out_texture_width, out_texture_height);

        // Get the texture data
        glReadPixels(0, 0, out_texture_width, out_texture_height, GL_DEPTH_COMPONENT, GL_FLOAT, texture_image.get_pixel_data());

        return texture_image;
    }

    /**
     * Set the mode to grab the nearest texture pixel, if texture pixels don't align with screen pixels
     * @return None
    */
    static void set_pixel_parameters_nearest() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    
    /**
     * Set the mode to interpolate between the nearest texture pixels, if texture pixels don't align with screen pixels
     * @return None
    */
    static void set_pixel_parameters_linear() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    void delete_texture() {
        if(!no_texture) {
            glDeleteTextures(1, &renderedTexture);
            no_texture = true;
        }
    }
};

class GPUImage {
public:
    GPUTexture texture;
    GPUImage() {

    }
    static GPUImage from_CPUImage(ImageRGBA_UChar cpu_image) {
        GPUImage new_gpu_image;
        new_gpu_image.texture = vicmil::GPUTexture::from_raw_image_rgba(cpu_image);
        return new_gpu_image;
    }
    /*
    Overwrite the previously loaded gpu image with new content
    */
    void overwrite_with_CPUImage(ImageRGBA_UChar cpu_image) {
        texture.overwrite_texture_data(cpu_image);
    }
    ImageRGBA_UChar to_CPUImage() {
        texture.start_render_to_texture(); // Must be rendering to the texture in order to get the image
        ImageRGBA_UChar new_cpu_image = texture.get_screenshot(texture._width, texture._height);
        texture.stop_rendering_to_texture();
        new_cpu_image.flip_vertical();
        return new_cpu_image;
    }
};

// ============================================================
//                      GPU Settings and setup
// ============================================================
void init_SDL() {
    if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)==-1)) { 
        ThrowError("Could not initialize SDL: %s.\n" << SDL_GetError());
    }
}

void quit_SDL() {
    SDL_Quit();
}

/**
 * Quit if requested by user
 * Update window to keep it alive
 * Fetch events since last update
*/ 
std::vector<SDL_Event> update_SDL() {
    std::vector<SDL_Event> events;
    SDL_Event event;
    while( SDL_PollEvent( &event ) ) {
        if( event.type == SDL_QUIT )
        {
            quit_SDL();
            throw;
        }
        events.push_back(event);
    }
    return events;
}

/**
 * Using blening in the frag shader makes the graphics look better by being less blocky
*/
void setup_frag_shader_blending() {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void create_window_and_renderer(int width, int height, SDL_Renderer** renderer, SDL_Window** window) {
    /* Create a windowed mode window and its OpenGL context */
    #ifdef __EMSCRIPTEN__
    // Resizing in emscripten makes the window flicker :(
    SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_OPENGL, window, renderer);
    #else
    SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE, window, renderer);
    #endif
}

void create_window(int winWidth, int winHeight, SDL_Window** window, std::string window_name = "Opengl window") {
    *window = SDL_CreateWindow(window_name.c_str(), 
                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, // Create the window in the middle of the screen by default
                        winWidth, winHeight, 
                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
}

class Window {
public:
    SDL_Window *window = NULL;
    SDL_GLContext gl_context;

    Window() {}
    Window(int width, int height, std::string window_name = "Hello Triangle Minimal") {
        vicmil::create_window(width, height, &window, window_name);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetSwapInterval(1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        // Create OpenGLES 2 context on SDL window
        gl_context = SDL_GL_CreateContext(window);

        #ifndef __EMSCRIPTEN__
            //Initialize GLEW
            glewExperimental = GL_TRUE; 
            GLenum glewError = glewInit();
            if( glewError != GLEW_OK )
            {
                printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
            }

            //Use Vsync
            if( SDL_GL_SetSwapInterval( 1 ) < 0 )
            {
                printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
            }
        #endif

        int winWidth; int winHeight;
        // Get actual GL window size in pixels, in case of high dpi scaling
        SDL_GL_GetDrawableSize(window, &winWidth, &winHeight);
        printf("INFO: GL window size = %dx%d\n", winWidth, winHeight);
        glViewport(0, 0, winWidth, winHeight);   

        printf("INFO: GL version: %s\n", glGetString(GL_VERSION));
    }
    void set_to_current_window() {
        #if defined(OS_Windows) || defined(__EMSCRIPTEN__)
            return; // Not supported
        #endif
        GLCall(SDL_GL_MakeCurrent(window, gl_context));
    }
    void show_on_screen() {
        GLCall(SDL_GL_SwapWindow(window));
    }
};

void clear_screen(ColorRGBA_UChar clear_color = ColorRGBA_UChar(0, 0, 0, 255)) {
    glClearColor(clear_color.r/255.0f, clear_color.g/255.0f, clear_color.b/255.0f, clear_color.a/255.0f);
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

// Clear all the color information from the screen
void clear_color_buffer() {
    GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

// Clear all the depth information from the screen
void clear_depth_buffer() {
    GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void set_depth_testing_enabled(bool is_enabled) {
    // This makes sure that things closer block things further away
    if(is_enabled) {
        // Enable depth buffer
        glEnable(GL_DEPTH_TEST);
    }
    else {
        glDisable(GL_DEPTH_TEST);
    }
}

/**
 * Create a vertex array object, there usually have to exist one of those for the program to work!
*/
GLuint create_vertex_array_object() {
    GLuint vao;
    #if defined(__EMSCRIPTEN__)
        return 0; // No need to create vertex array object
    #elif defined(__MINGW64__)
        return 0; // No need to create vertex array object
        //glGenVertexArraysOES(1, &vao);
        //glBindVertexArrayOES(vao);
    #else
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    #endif
    return vao;
}

// Specify where on the screen to render
void set_viewport(int x, int y, int w, int h) {
    GLCall(glViewport(x, y, w, h));
}

// ============================================================
//                      User Input
// ============================================================
/**
 * Create a screenshot of the current keyboard state, with what keys are pressed down
*/
class KeyboardState {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
public:
    bool key_is_pressed(unsigned int sdl_key) const {
        return (bool)state[sdl_key];
    }
    bool escape_key_is_pressed() const {
        return key_is_pressed(SDL_SCANCODE_ESCAPE);
    }
};

#ifdef __EMSCRIPTEN__
// Function to get window dimensions
void get_window_size(SDL_Window *window, int &width, int &height) {
    width = EM_ASM_INT({ return window.innerWidth; });
    height = EM_ASM_INT({ return window.innerHeight; });
    int internal_width, internal_height;
    SDL_GL_GetDrawableSize(window, &internal_width, &internal_height);
    vicmil::set_viewport(0, 0, internal_width, internal_height); // You often want to update the viewport too, so do it just in case
}
#else
void get_window_size(SDL_Window *window, int &width, int &height) {
    SDL_GL_GetDrawableSize(window, &width, &height);
    vicmil::set_viewport(0, 0, width, height); // You often want to update the viewport too, so do it just in case
}
#endif

/**
 * Create a screenshot of the current mouse state, with mouse position and button presses
*/
class MouseState {
public:
    int _x; // The x position in pixels
    int _y; // The y position in pixels
    Uint32 _button_state;
    MouseState(SDL_Window *window) { // May not work if update_SDL has not been called for a while
        _button_state = SDL_GetMouseState(&_x, &_y);

        // If there is a discrepancy between the internal window size of sdl and outside(such as when running in the browser)
        //   then we may need to update the position of the mouse to get the "actual position"
        #ifdef __EMSCRIPTEN__
            int internal_width, internal_height;
            SDL_GL_GetDrawableSize(window, &internal_width, &internal_height);
            int external_width, external_height;
            get_window_size(window, external_width, external_height);
            _x = _x * double(external_width) / internal_width;
            _y = _y * double(external_height) / internal_height;
        #endif
    }
    int x() const {
        return _x;
    }
    int y() const {
        return _y;
    }
    bool left_button_is_pressed() const {
        return (bool)(_button_state & SDL_BUTTON(1));
    }
    bool middle_button_is_pressed() const {
        return (bool)(_button_state & SDL_BUTTON(2));
    }
    bool right_button_is_pressed() const {
        return (bool)(_button_state & SDL_BUTTON(3));
    }
};

/**
 * Determine if the mouse left button has been pressed:
 * Read here for more documentation: 
 * https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlmousebuttonevent.html
*/
bool mouse_left_clicked(const std::vector<SDL_Event>& events) {
    auto it = events.begin();
    while(it != events.end()) {
        const SDL_Event& event = *it;
        if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            return true;
        }
        it++;
    }
    return false;
} 

/**
 * Determine if the mouse right button has been pressed:
 * Read here for more documentation: 
 * https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlmousebuttonevent.html
*/
bool mouse_right_clicked(const std::vector<SDL_Event>& events) {
    auto it = events.begin();
    while(it != events.end()) {
        const SDL_Event& event = *it;
        if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
            return true;
        }
        it++;
    }
    return false;
} 


/**
 * Determine if the window has been resized:
*/
bool window_resized(const std::vector<SDL_Event>& events) {
    for(int i = 0; i < events.size(); i++) {
        SDL_Event event = events[i];
        if(event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_RESIZED) {
            return true;
        }
    }
    return false;
} 

/**
 * Handle text input from the user:
*/
class TextInput {
public:
    std::vector<int> inputText;  // Store input as Unicode code points

    // In some languages, such as japanese or chinese, the user may want to type multiple characters to select which character to input
    // The user is still deciding which character to input 
    // Append at the end of the text, then the actual input is handled as a text input once the user is done with the selection
    std::vector<int> compositionText;  // Store composition text as Unicode code points

    size_t cursorPos = 0;  // Track the cursor position (index into inputText)

    bool update(const std::vector<SDL_Event>& events) {
        bool updated = false;

        for (int i = 0; i < events.size(); i++) {
            const SDL_Event& event = events[i];
            if (event.type == SDL_TEXTINPUT) {
                // Convert the UTF-8 text input to Unicode code points
                std::vector<int> codePoints = vicmil::utf8ToUnicodeCodePoints(event.text.text);
                
                // Insert the input text at the cursor position
                inputText.insert(inputText.begin() + cursorPos, codePoints.begin(), codePoints.end());
                cursorPos += codePoints.size();  // Move the cursor to the end of the newly inserted text
                compositionText.clear();  // Clear composition text once input is confirmed
                updated = true;
            } else if (event.type == SDL_TEXTEDITING) {
                // Convert the composition text to Unicode code points and update
                compositionText = vicmil::utf8ToUnicodeCodePoints(event.edit.text);
                updated = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && cursorPos > 0) {
                    // Remove character before cursor position
                    inputText.erase(inputText.begin() + cursorPos - 1);
                    cursorPos--;  // Move cursor left
                    updated = true;
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    // Handle enter (newline)
                    std::vector<int> newline_char = {'\n'};
                    inputText.insert(inputText.begin() + cursorPos, newline_char.begin(), newline_char.end());
                    cursorPos++;  // Move cursor to end after newline
                    compositionText.clear();
                    updated = true;
                } else if (event.key.keysym.sym == SDLK_LEFT && cursorPos > 0) {
                    // Move cursor left
                    cursorPos--;
                    updated = true;
                } else if (event.key.keysym.sym == SDLK_RIGHT && cursorPos < inputText.size()) {
                    // Move cursor right
                    cursorPos++;
                    updated = true;
                } else if (event.key.keysym.sym == SDLK_v && (SDL_GetModState() & KMOD_CTRL)) {
                    // Handle Ctrl+V (paste from clipboard)
                    const char* clipboardText = SDL_GetClipboardText();
                    if (clipboardText) {
                        // Convert clipboard text to Unicode code points
                        std::vector<int> clipboardCodePoints = vicmil::utf8ToUnicodeCodePoints(clipboardText);
                        
                        // Insert clipboard text at the cursor position
                        inputText.insert(inputText.begin() + cursorPos, clipboardCodePoints.begin(), clipboardCodePoints.end());
                        cursorPos += clipboardCodePoints.size();  // Move cursor to the end of the pasted text
                        updated = true;
                    }
                }
            }
        }

        return updated;
    }

    void start() {
        SDL_StartTextInput();
        // If running in the browser with emscripten, you also want to create an empty input element to handle input, especially for mobile devices
        // TODO
    }

    void stop() {
        SDL_StopTextInput();
    }

    // Function to get the UTF-8 encoded string of inputText
    std::string getInputTextUTF8() const {
        return vicmil::unicodeToUtf8(inputText);
    }

    // Function to get the UTF-8 encoded string of compositionText
    std::string getCompositionTextUTF8() const {
        return vicmil::unicodeToUtf8(compositionText);
    }

    // Function to get the UTF-8 encoded string of inputText with cursor indicator '|'
    std::string getInputTextUTF8WithCursor() {
        size_t cursorPos = getCursorPos();

        // Ensure the cursor position is within bounds
        if (cursorPos > inputText.size()) {
            cursorPos = inputText.size();
        }

        std::vector<int> inputText_copy = inputText;
        std::vector<int> cursor_indicator = {'|'};
        inputText_copy.insert(inputText_copy.begin() + cursorPos, cursor_indicator.begin(), cursor_indicator.end());

        return vicmil::unicodeToUtf8(inputText_copy);
    }

    // Function to get the current cursor position (for rendering or other purposes)
    size_t getCursorPos() const {
        return cursorPos;
    }
};


// ============================================================
//                    Gui and layout
// ============================================================
class GuiEngine {
public:
    static const int _________ = 0;
    static const int _e_right_ = 1;
    static const int _e_bottom = 2;
    static const int _o_right_ = 4;
    static const int _o_bottom = 8;
    
    enum attach_location {
        // Make two elements meet at corner
        // Element to attach to(other), Element
        o_BottomRight_e_BottomRight = _o_bottom|_o_right_|_e_bottom|_e_right_,
        o_BottomRight_e_BottomLeft =  _o_bottom|_o_right_|_e_bottom|_________,
        o_BottomRight_e_TopRight =    _o_bottom|_o_right_|_________|_e_right_,
        o_BottomRight_e_TopLeft =     _o_bottom|_o_right_|_________|_________,
        o_BottomLeft_e_BottomRight =  _o_bottom|_________|_e_bottom|_e_right_,
        o_BottomLeft_e_BottomLeft =   _o_bottom|_________|_e_bottom|_________,
        o_BottomLeft_e_TopRight =     _o_bottom|_________|_________|_e_right_,
        o_BottomLeft_e_TopLeft =      _o_bottom|_________|_________|_________,
        o_TopRight_e_BottomRight =    _________|_o_right_|_e_bottom|_e_right_,
        o_TopRight_e_BottomLeft =     _________|_o_right_|_e_bottom|_________,
        o_TopRight_e_TopRight =       _________|_o_right_|_________|_e_right_,
        o_TopRight_e_TopLeft =        _________|_o_right_|_________|_________,
        o_TopLeft_e_BottomRight =     _________|_________|_e_bottom|_e_right_,
        o_TopLeft_e_BottomLeft =      _________|_________|_e_bottom|_________,
        o_TopLeft_e_TopRight =        _________|_________|_________|_e_right_,
        o_TopLeft_e_TopLeft =         _________|_________|_________|_________,
        null_
    };
    struct Attachment {
        int w = 0;
        int h = 0;
        std::string attach_to = "";
        attach_location attach_loc = attach_location::null_;
        int layer = -1;
        Attachment() {};
        Attachment(int w_, int h_, std::string attach_to_, attach_location attach_loc_, int layer_) {
            w = w_;
            h = h_;
            attach_to = attach_to_;
            attach_loc = attach_loc_;
            layer = layer_;
        }
    };
    struct Rect {
        int x;
        int y;
        int w;
        int h;
        Rect() {}
        Rect(int x_, int y_, int w_, int h_) {
            x = x_;
            y = y_;
            w = w_;
            h = h_;
        }
        bool inside_rect(int x_, int y_) {
            if(x_ > x && y_ > y && x_ < x + w && y_ < y + h) {
                return true;
            }
            return false;
        }
        std::string to_string() {
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(w) + ", " + std::to_string(h) + ")";
        }
    };
    struct RectGL {
        float x;
        float y;
        float w;
        float h;
        RectGL() {}
        RectGL(float x_, float y_, float w_, float h_) {
            x = x_;
            y = y_;
            w = w_;
            h = h_;
        }
        std::string to_string() {
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(w) + ", " + std::to_string(h) + ")";
        }
    };
    int _screen_w = 0;
    int _screen_h = 0;
    std::map<std::string, Attachment> _attachments = std::map<std::string, Attachment>();
    std::map<std::string, Rect> _attachment_pos = std::map<std::string, Rect>();

    void set_screen_size(int w, int h) {
        _screen_w = w;
        _screen_h = h;
    }
    void element_attach(std::string name, int w, int h, std::string attach_to, attach_location attach_loc, int layer=0) {
        _attachments[name] = Attachment(w, h, attach_to, attach_loc, layer);
    }
    void element_remove(std::string name) {
        _attachments.erase(name);
    }
    bool contains_element(std::string name) {
        return _attachments.count(name) == 0;
    }
    void clear() { // Remove all elements
        _attachments.clear();
        _attachment_pos.clear();
    }
    std::vector<std::string> element_list() {
        std::vector<std::string> keys;
        keys.reserve(_attachments.size());

        // Extract keys
        for (const auto& pair : _attachments) {
            keys.push_back(pair.first);
        }

        return keys;
    }
    Attachment get_element_attachment(std::string name) {
        return _attachments[name];
    }
    // update positions based on attachments
    void build() {
        _attachment_pos.clear();
        _attachment_pos["screen"] = Rect(0, 0, _screen_w, _screen_h);
        std::vector<std::string> elements = element_list();
        std::set<std::string> elements_to_update = std::set<std::string>(elements.begin(), elements.end());
        bool recieved_update = true;
        std::vector<std::string> new_elements = {};
        while(recieved_update) { // Iterate while elements are still being updated
            recieved_update = false;
            for (std::string name : elements_to_update) { // Iterates directly over elements
                Attachment& attach_ = _attachments[name];
                if(_attachment_pos.count(attach_.attach_to) == 0) {
                    continue; // The element to attach to have not been given its position
                }
                new_elements.push_back(name);
                recieved_update = true;
                // Assign a position to the element
                Rect other = _attachment_pos[attach_.attach_to];
                Rect attach_pos = Rect(other.x, other.y, attach_.w, attach_.h);
                attach_pos.x = other.x 
                    + bool(attach_.attach_loc&_o_right_) * other.w
                    - bool(attach_.attach_loc&_e_right_) * attach_.w;
                attach_pos.y = other.y 
                    + bool(attach_.attach_loc&_o_bottom) * other.h
                    - bool(attach_.attach_loc&_e_bottom) * attach_.h;
                _attachment_pos[name] = attach_pos;
            }
            // Removes elements that have already been added, so we don't have to look at them again
            for (std::string new_elem : new_elements) {
                elements_to_update.erase(new_elem); // Removes the element if it exists, does nothing otherwise
            }
            new_elements.clear();
        }
    }
    // get which element is located at position
    std::string get_xy_element(int x_, int y_) {
        // Naive approach. Just iterate through all the elements and find which one has the highest layer value, where (x,y) lies inside it
        std::string attachment = "screen";
        int attachment_layer = -1;
        for (const auto& pair : _attachment_pos) {
            vicmil::GuiEngine::Rect rect = pair.second;
            if(rect.inside_rect(x_, y_) && _attachments[pair.first].layer > attachment_layer) {
                attachment = pair.first;
                attachment_layer = _attachments[pair.first].layer;
            }
        }
        return attachment;
    }
    Rect get_element_pos(std::string name) {
        if(_attachment_pos.count(name) == 0) {
            return Rect(0, 0, 0, 0);
        }
        return _attachment_pos[name];
    }
    // pos in [-1, 1]
    RectGL rect_to_rect_gl(Rect rect) {
        RectGL gl_pos;
        gl_pos.x = (float(rect.x) / _screen_w) * 2.0f - 1.0f;
        gl_pos.y = -((float(rect.y) / _screen_h) * 2.0f - 1.0f); // The y = -1 is at the bottom of the screen
        gl_pos.w = 2.0f * (float(rect.w) / _screen_w);
        gl_pos.h = 2.0f * (float(rect.h) / _screen_h);
        return gl_pos;
    }
    // pos in [-1, 1]
    RectGL get_element_gl_pos(std::string name) {
        if(_attachment_pos.count(name) == 0) {
            return RectGL(0, 0, 0, 0);
        }
        return rect_to_rect_gl(_attachment_pos[name]);
    }
};

class TextureLayout {
public:
    TextureLayout() {}
    std::map<std::string, GuiEngine::Rect> _element_pos = std::map<std::string, GuiEngine::Rect>();
    int _texture_width = 0;
    int _texture_height = 0;
    void set_texture_size(int w, int h) {
        _texture_width = w;
        _texture_height = h;
    }
    void add_element(std::string name, int x, int y, int w, int h) {
        _element_pos[name] = GuiEngine::Rect(x, y, w, h);
    }
    void remove_element(std::string name) {
        _element_pos.erase(name);
    }
    bool contains_element(std::string name) {
        return _element_pos.count(name) != 0;
    }
    std::vector<std::string> element_list() {
        std::vector<std::string> keys;
        keys.reserve(_element_pos.size());

        // Extract keys
        for (const auto& pair : _element_pos) {
            keys.push_back(pair.first);
        }

        return keys;
    }
    GuiEngine::Rect get_element_pos(std::string name) {
        return _element_pos[name];
    }
    GuiEngine::RectGL get_element_gl_pos(std::string name) {
        GuiEngine::Rect pos = _element_pos[name];
        GuiEngine::RectGL gl_pos;
        gl_pos.x = (float(pos.x) / _texture_width);
        gl_pos.y = (float(pos.y) / _texture_height); // The y = 0 is at the bottom of the screen
        gl_pos.w = float(pos.w) / _texture_width;
        gl_pos.h =  float(pos.h) / _texture_height;
        return gl_pos;
    }
};

// ============================================================
//                    Default gpu programs (i.e. shaders)
// ============================================================


// Add a 2d rectangle with a single color to the things to draw
inline void add_color_rect_to_triangle_buffer(
    std::vector<vicmil::VertexCoordColor>& vertices, 
    vicmil::GuiEngine::RectGL layout_pos, 
    unsigned int layer, // supports over 1 000 000 layers, where the higher layers will be drawn first
    unsigned char r, 
    unsigned char g, 
    unsigned char b,
    unsigned char a = 255) {
    vertices.push_back(vicmil::VertexCoordColor(layout_pos.x, layout_pos.y,                              1.0f - (layer+1)/2000000.0f, r/255.0, g/255.0, b/255.0, a/255.0));
    vertices.push_back(vicmil::VertexCoordColor(layout_pos.x + layout_pos.w, layout_pos.y,               1.0f - (layer+1)/2000000.0f, r/255.0, g/255.0, b/255.0, a/255.0));
    vertices.push_back(vicmil::VertexCoordColor(layout_pos.x, layout_pos.y - layout_pos.h,               1.0f - (layer+1)/2000000.0f, r/255.0, g/255.0, b/255.0, a/255.0));
    vertices.push_back(vicmil::VertexCoordColor(layout_pos.x + layout_pos.w, layout_pos.y - layout_pos.h,1.0f - (layer+1)/2000000.0f, r/255.0, g/255.0, b/255.0, a/255.0));
    vertices.push_back(vicmil::VertexCoordColor(layout_pos.x + layout_pos.w, layout_pos.y,               1.0f - (layer+1)/2000000.0f, r/255.0, g/255.0, b/255.0, a/255.0));
    vertices.push_back(vicmil::VertexCoordColor(layout_pos.x, layout_pos.y - layout_pos.h,               1.0f - (layer+1)/2000000.0f, r/255.0, g/255.0, b/255.0, a/255.0));
}

// Add a 2d rectangle with an image on it to the things to draw
inline void add_texture_rect_to_triangle_buffer(
    std::vector<vicmil::VertexTextureCoord>& vertices, 
    vicmil::GuiEngine::RectGL layout_pos, // The position of the rectangle
    unsigned int layer, // supports over 1 000 000 layers, where the higher layers will be drawn first
    vicmil::GuiEngine::RectGL texture_pos = vicmil::GuiEngine::RectGL(0, 0, 1, 1)) {
    vertices.push_back(vicmil::VertexTextureCoord(layout_pos.x, 
                                                  layout_pos.y,                              1.0f - (layer+1)/2000000.0f, 
                                                  texture_pos.x, 
                                                  texture_pos.y));
    vertices.push_back(vicmil::VertexTextureCoord(layout_pos.x + layout_pos.w, 
                                                  layout_pos.y,                              1.0f - (layer+1)/2000000.0f, 
                                                  texture_pos.x + texture_pos.w, 
                                                  texture_pos.y));
    vertices.push_back(vicmil::VertexTextureCoord(layout_pos.x, 
                                                  layout_pos.y - layout_pos.h,               1.0f - (layer+1)/2000000.0f, 
                                                  texture_pos.x, 
                                                  texture_pos.y + texture_pos.h));

    vertices.push_back(vicmil::VertexTextureCoord(layout_pos.x + layout_pos.w, 
                                                  layout_pos.y - layout_pos.h,               1.0f - (layer+1)/2000000.0f, 
                                                  texture_pos.x + texture_pos.w, 
                                                  texture_pos.y + texture_pos.h));
    vertices.push_back(vicmil::VertexTextureCoord(layout_pos.x + layout_pos.w, 
                                                  layout_pos.y,                              1.0f - (layer+1)/2000000.0f, 
                                                  texture_pos.x + texture_pos.w, 
                                                  texture_pos.y));
    vertices.push_back(vicmil::VertexTextureCoord(layout_pos.x, 
                                                  layout_pos.y - layout_pos.h,               1.0f - (layer+1)/2000000.0f, 
                                                  texture_pos.x, 
                                                  texture_pos.y + texture_pos.h));
}

class DefaultGpuPrograms {
public:
    vicmil::GPUProgram gpu_program_VertexCoordColor_no_proj;   // no projection using uniform buffer matrix
    vicmil::GPUProgram gpu_program_VertexCoordColor_proj;      // with projection using uniform buffer matrix
    vicmil::GPUProgram gpu_program_VertexTextureCoord_no_proj; // no projection using uniform buffer matrix
    vicmil::GPUProgram gpu_program_VertexTextureCoord_proj;    // with projection using uniform buffer matrix

    vicmil::VertexBuffer default_vertex_buffer;
    vicmil::UniformBufferMat4f default_uniform_buffer;

    static void set_vertex_buffer_layout_VertexCoordColor(vicmil::GPUProgram& gpu_program) {
        auto position_layout_element = vicmil::VertexBufferElement(GL_FLOAT, 3, "position_in", GL_FALSE); // x, y, z
        auto tex_coord_layout_element = vicmil::VertexBufferElement(GL_FLOAT, 4, "color_in", GL_FALSE); // r, g, b, a
        vicmil::VertexBufferLayout vertex_layout = vicmil::VertexBufferLayout({position_layout_element, tex_coord_layout_element});
        vertex_layout.set_vertex_buffer_layout(gpu_program);
    }
    static void set_vertex_buffer_layout_VertexTextureCoord(vicmil::GPUProgram& gpu_program) {
        auto position_layout_element = vicmil::VertexBufferElement(GL_FLOAT, 3, "position_in", GL_FALSE); // x, y, z
        auto tex_coord_layout_element = vicmil::VertexBufferElement(GL_FLOAT, 2, "texcoord_in", GL_FALSE); // u, v
        vicmil::VertexBufferLayout vertex_layout = vicmil::VertexBufferLayout({position_layout_element, tex_coord_layout_element});
        vertex_layout.set_vertex_buffer_layout(gpu_program);
    }
    void init_VertexCoordColor_no_proj() {
        gpu_program_VertexCoordColor_no_proj = vicmil::GPUProgram::from_strings(vicmil::VertexCoordColor::gles_no_proj_vert_src, vicmil::VertexCoordColor::gles_frag_src);
        gpu_program_VertexCoordColor_no_proj.bind_program();
    }
    void init_VertexCoordColor_proj() {
        gpu_program_VertexCoordColor_proj = vicmil::GPUProgram::from_strings(vicmil::VertexCoordColor::gles_vert_src, vicmil::VertexCoordColor::gles_frag_src);
        gpu_program_VertexCoordColor_proj.bind_program();
    }
    void init_VertexTextureCoord_no_proj() {
        gpu_program_VertexTextureCoord_no_proj = vicmil::GPUProgram::from_strings(vicmil::VertexTextureCoord::gles_no_proj_vert_src, vicmil::VertexTextureCoord::gles_frag_src);
        gpu_program_VertexTextureCoord_no_proj.bind_program();
    }
    void init_VertexTextureCoord_proj() {
        gpu_program_VertexTextureCoord_proj = vicmil::GPUProgram::from_strings(vicmil::VertexTextureCoord::gles_vert_src, vicmil::VertexTextureCoord::gles_frag_src);
        gpu_program_VertexTextureCoord_proj.bind_program();
    }
    void init_default_gpu_programs() {
        /*
        Initialize programs to be run on the gpu, i.e. shaders
        */
        // Used for drawing single-color shapes, uses VertexCoordColor which specifies color and position of triangles
        init_VertexCoordColor_no_proj(); // Just draw to the screen
        init_VertexCoordColor_proj(); // Add perspective and camera position to draw things in 3d using a uniform buffer

        // Used for drawing shapes with images, uses VertexTextureCoord which specifies where on the texure to draw and position of triangles
        init_VertexTextureCoord_no_proj(); // Just draw to the screen
        init_VertexTextureCoord_proj(); // Add perspective and camera position to draw things in 3d using a uniform buffer

        // Create a default vertex buffer
        default_vertex_buffer = create_vertex_buffer();

        // Create default uniform buffer
        default_uniform_buffer = vicmil::UniformBufferMat4f("u_MVP");

        // Enable depth testing(so that things in front are drawn infront, otherwise there might be undefined behaviour if shapes overlap)
        vicmil::set_depth_testing_enabled(true);


        // Make textures with an alpha value less than one more transperant
        // Note! Not setting any value for blending results in undefined bahaviour when alpha values are less than 1
        vicmil::setup_frag_shader_blending(); 
    }

    static vicmil::VertexBuffer create_vertex_buffer() {
        // Provide a default vertex buffer so the user don't have to think about it
        std::vector<VertexCoordColor> vertex_buffer_contents = {};
        add_color_rect_to_triangle_buffer(vertex_buffer_contents, GuiEngine::RectGL(-0.5, 0.5, 1, 1), 0, 255, 0, 0);
        vicmil::VertexBuffer vertex_buffer = vicmil::VertexBuffer::from_vertex_vector(vertex_buffer_contents);
        return vertex_buffer;
    }

    void draw_2d_VertexCoordColor_vertex_buffer(std::vector<vicmil::VertexCoordColor>& vertices) {
        gpu_program_VertexCoordColor_no_proj.bind_program();
        vicmil::DefaultGpuPrograms::set_vertex_buffer_layout_VertexCoordColor(gpu_program_VertexCoordColor_no_proj);
        default_vertex_buffer.bind();
        default_vertex_buffer.overwrite_vertex_vector(vertices);
        default_vertex_buffer.draw_triangles();
    }
    void draw_2d_VertexTextureCoord_vertex_buffer(std::vector<vicmil::VertexTextureCoord>& vertices, vicmil::GPUImage gpu_image) {
        gpu_program_VertexTextureCoord_no_proj.bind_program();
        vicmil::DefaultGpuPrograms::set_vertex_buffer_layout_VertexTextureCoord(gpu_program_VertexTextureCoord_no_proj);
        gpu_image.texture.bind();
        default_vertex_buffer.bind();
        default_vertex_buffer.overwrite_vertex_vector(vertices);
        default_vertex_buffer.draw_triangles();
    }
    void draw_3d_VertexCoordColor_vertex_buffer(std::vector<vicmil::VertexCoordColor>& vertices, glm::mat4 transform_matrix) {
        gpu_program_VertexCoordColor_proj.bind_program();
        vicmil::DefaultGpuPrograms::set_vertex_buffer_layout_VertexCoordColor(gpu_program_VertexCoordColor_proj);
        default_vertex_buffer.bind();
        default_vertex_buffer.overwrite_vertex_vector(vertices);
        default_uniform_buffer.set_matrix(transform_matrix, gpu_program_VertexCoordColor_proj.id);
        default_vertex_buffer.draw_triangles();
    }
    void draw_3d_VertexCoordColor_vertex_buffer_as_points(std::vector<vicmil::VertexCoordColor>& vertices, glm::mat4 transform_matrix) {
        gpu_program_VertexCoordColor_proj.bind_program();
        vicmil::DefaultGpuPrograms::set_vertex_buffer_layout_VertexCoordColor(gpu_program_VertexCoordColor_proj);
        default_vertex_buffer.bind();
        default_vertex_buffer.overwrite_vertex_vector(vertices);
        default_uniform_buffer.set_matrix(transform_matrix, gpu_program_VertexCoordColor_proj.id);
        default_vertex_buffer.draw_points();
    }
    void draw_3d_VertexTextureCoord_vertex_buffer(std::vector<vicmil::VertexTextureCoord>& vertices, vicmil::GPUImage gpu_image, glm::mat4 transform_matrix) {
        gpu_program_VertexTextureCoord_proj.bind_program();
        vicmil::DefaultGpuPrograms::set_vertex_buffer_layout_VertexTextureCoord(gpu_program_VertexTextureCoord_proj);
        gpu_image.texture.bind();
        default_vertex_buffer.bind();
        default_vertex_buffer.overwrite_vertex_vector(vertices);
        default_uniform_buffer.set_matrix(transform_matrix, gpu_program_VertexTextureCoord_proj.id);
        default_vertex_buffer.draw_triangles();
    }
};
}