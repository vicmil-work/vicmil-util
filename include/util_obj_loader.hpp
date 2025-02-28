#pragma once

#include "util_std.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

namespace vicmil {
struct Normal {
    float x = -100;
    float y = -100;
    float z = -100;
    Normal() {}
    Normal(float x_, float y_, float z_) {
        x = x_;
        y = y_;
        z = z_;
    }
};

struct TexCoord {
    float u = -100;
    float v = -100;
    TexCoord() {}
    TexCoord(float u_, float v_) {
        u = u_;
        v = v_;
    }
};

struct VertexCoord {
    float x = -100;
    float y = -100;
    float z = -100;
    VertexCoord() {}
    VertexCoord(float x_, float y_, float z_) {
        x = x_;
        y = y_;
        z = z_;
    }
};

struct Vertex {
    VertexCoord vertex_cord;
    TexCoord tex_cord;
    Normal norm;
    unsigned int material_id;
};

struct Face {
    int vertex_indices[3];
};

struct Material {
    std::string name;
    std::string diffuse_texname;  // Texture map_Kd, this is the main texture that will be drawn on the object
    std::string ambient_texname;  // Texture map_Ka
    std::string specular_texname; // Texture map_Ks
    std::string bump_texname;     // Texture bump
    std::string emissive_texname; // Emissive texture map_Ke

    // Material properties
    float Ka[3] = {0.2f, 0.2f, 0.2f};   // Ambient reflectivity
    float Kd[3] = {0.8f, 0.8f, 0.8f};   // Diffuse reflectivity # This is the default color that will be shown
    float Ks[3] = {1.0f, 1.0f, 1.0f};   // Specular reflectivity
    float Ke[3] = {0.0f, 0.0f, 0.0f};   // Emissive color
    float Kr[3] = {0.0f, 0.0f, 0.0f};   // Reflection color
    float Ns = 0.0f;      // Specular exponent (shininess)
    float Ni = 1.0f;      // Index of refraction
    float d = 1.0f;       // Transparency (0.0 = fully transparent, 1.0 = fully opaque)
    int illum = 2;        // Illumination model
    float metallic = 0.0f; // Metallic factor (PBR)
    float roughness = 0.0f; // Roughness factor (PBR)
};

struct VertexCoordColor {
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;
    float r = 0.0;
    float g = 1.0;
    float b = 0.0;
    float a = 1.0;
    VertexCoordColor() {}
    VertexCoordColor(float x_, float y_, float z_, float r_, float g_, float b_, float a_ = 1.0) {
        x = x_;
        y = y_;
        z = z_;
        r = r_;
        g = g_;
        b = b_;
        a = a_;
    }
    // Vertex shader
    static constexpr const char* gles_vert_src =
        "uniform mat4 u_MVP;                                   \n"
        "attribute vec4 position_in;                           \n"
        "attribute vec4 color_in;                              \n"
        "varying vec4 color;                                   \n"
        "void main()                                           \n"
        "{                                                     \n"
        "    gl_Position = u_MVP * vec4(position_in.xyz, 1.0); \n"
        "    color = color_in;                                 \n"
        "}                                                     \n";

    // Vertex shader without projection
    static constexpr const char* gles_no_proj_vert_src =
        "attribute vec4 position_in;                           \n"
        "attribute vec4 color_in;                              \n"
        "varying vec4 color;                                   \n"
        "void main()                                           \n"
        "{                                                     \n"
        "    gl_Position = vec4(position_in.xyz, 1.0);         \n"
        "    color = color_in;                                 \n"
        "}                                                     \n";

    // Fragment/pixel shader
    static constexpr const char* gles_frag_src =
        "precision mediump float;                     \n"
        "varying vec4 color;                          \n"
        "void main()                                  \n"
        "{                                            \n"
        "    gl_FragColor = color;                    \n"
        "}                                            \n";
};

struct VertexTextureCoord {
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;
    float u = 0.0;
    float v = 1.0;
    VertexTextureCoord() {}
    VertexTextureCoord(float x_, float y_, float z_, float u_, float v_) {
        x = x_;
        y = y_;
        z = z_;
        u = u_;
        v = v_;
    }
    // Vertex shader
    static constexpr const char* gles_vert_src =
        "uniform mat4 u_MVP;                                   \n"
        "attribute vec4 position_in;                           \n"
        "attribute vec2 texcoord_in;                           \n"
        "varying vec2 tex_coord;                               \n"
        "void main()                                           \n"
        "{                                                     \n"
        "    gl_Position = u_MVP * vec4(position_in.xyz, 1.0); \n"
        "    tex_coord = texcoord_in.xy;                       \n"
        "}                                                     \n";
    // Vertex shader without projection
    static constexpr const char* gles_no_proj_vert_src =
        "attribute vec4 position_in;                           \n"
        "attribute vec2 texcoord_in;                           \n"
        "varying vec2 tex_coord;                               \n"
        "void main()                                           \n"
        "{                                                     \n"
        "    gl_Position = vec4(position_in.xyz, 1.0);         \n"
        "    tex_coord = texcoord_in.xy;                       \n"
        "}                                                     \n";
    // Fragment/pixel shader
    static constexpr const char* gles_frag_src =
        "precision mediump float;                     \n"
        "varying vec2 tex_coord;                      \n"
        "uniform sampler2D our_texture;               \n"
        "void main()                                  \n"
        "{                                            \n"
        "  gl_FragColor = texture2D(our_texture, tex_coord); \n"
        "}                                            \n";
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<Material> materials;

    void shift_texture_coords(int material_id_, float old_x, float old_y, float old_w, float old_h, float new_x, float new_y, float new_w, float new_h) {
        // Since you may want to put multiple textures at the same time, and they may no longer take up the entire surface, the texture positions may need to be updated
        // The positions passed in refers to the position of the texture
        for(int i = 0; i < vertices.size(); i++) {
            if(vertices[i].material_id != material_id_) {
                continue;
            }
            float old_u = vertices[i].tex_cord.u;
            float old_v = vertices[i].tex_cord.v;
            vertices[i].tex_cord.u = (((old_u - old_x) * old_w) / new_w) + new_x;
            vertices[i].tex_cord.v = (((old_v - old_y) * old_h) / new_h) + new_y;
        }
    }

    std::vector<VertexCoord> get_vertex_coordinates() { // float x, float y, float z
        std::vector<VertexCoord> ret_vec = std::vector<VertexCoord>();
        ret_vec.reserve(vertices.size());
        for(int i = 0; i < vertices.size(); i++) {
            ret_vec.push_back(vertices[i].vertex_cord);
        }
        return ret_vec;
    }
    std::vector<VertexTextureCoord> get_vertex_texture_coordinates() { // float x, float y, float z, float u, float v
        std::vector<VertexTextureCoord> ret_vec = std::vector<VertexTextureCoord>();
        ret_vec.reserve(vertices.size());
        for(int i = 0; i < vertices.size(); i++) {
            Vertex& v = vertices[i];
            ret_vec.push_back(VertexTextureCoord(v.vertex_cord.x, v.vertex_cord.y, v.vertex_cord.z, v.tex_cord.u, v.tex_cord.v));
        }
        return ret_vec;
    }
    std::vector<VertexCoordColor> get_vertex_coordinates_colors() { // float x, float y, float z, float r, float g, float b
        std::vector<VertexCoordColor> ret_vec = std::vector<VertexCoordColor>();
        ret_vec.reserve(vertices.size());
        for(int i = 0; i < vertices.size(); i++) {
            if(vertices[i].material_id >= materials.size()) {
                ThrowError("Invalid material id! " << vertices[i].material_id);
            }
            Material& material_ = materials[vertices[i].material_id];
            float r = material_.Kd[0];
            float g = material_.Kd[1];
            float b = material_.Kd[2];
            VertexCoord coord = vertices[i].vertex_cord;
            ret_vec.push_back(VertexCoordColor(coord.x, coord.y, coord.z, r, g, b));
        }
        return ret_vec;
    }
    std::vector<Face>& get_vertex_face_indicies() { // int vert1, int vert2, int vert3
        return faces;
    }
    int get_triangle_count() {
        return faces.size();
    }
    VertexCoord get_avarage_vert_coord() {
        double x = 0;
        double y = 0;
        double z = 0;
        for(int i = 0; i < vertices.size(); i++) {
            x += vertices[i].vertex_cord.x;
            y += vertices[i].vertex_cord.y;
            z += vertices[i].vertex_cord.z;
        }
        VertexCoord avg_coord = VertexCoord(x / vertices.size(), y / vertices.size(), z / vertices.size());
        return avg_coord;
    }
};

struct _RawMesh {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
};

Mesh _load_obj_file(_RawMesh& raw_mesh) {
    tinyobj::attrib_t& attrib = raw_mesh.attrib;
    std::vector<tinyobj::shape_t>& shapes = raw_mesh.shapes;
    std::vector<tinyobj::material_t>& materials = raw_mesh.materials;
    
    Mesh mesh;

    // Load materials from the .mtl file
    mesh.materials.resize(materials.size());
    for (size_t i = 0; i < materials.size(); ++i) {
        const auto& mat = materials[i];
        Material material;
        material.name = mat.name;

        material.Ka[0] = mat.ambient[0];
        material.Ka[1] = mat.ambient[1];
        material.Ka[2] = mat.ambient[2];

        material.Kd[0] = mat.diffuse[0];
        material.Kd[1] = mat.diffuse[1];
        material.Kd[2] = mat.diffuse[2];

        material.Ks[0] = mat.specular[0];
        material.Ks[1] = mat.specular[1];
        material.Ks[2] = mat.specular[2];

        material.Ns = mat.shininess;

        material.Ni = mat.ior;

        material.d = mat.dissolve;

        material.illum = mat.illum;

        material.metallic = mat.metallic;
        material.roughness = mat.roughness;

        if (!mat.diffuse_texname.empty()) {
            material.diffuse_texname = mat.diffuse_texname;
        }
        if (!mat.ambient_texname.empty()) {
            material.ambient_texname = mat.ambient_texname;
        }
        if (!mat.specular_texname.empty()) {
            material.specular_texname = mat.specular_texname;
        }
        if (!mat.bump_texname.empty()) {
            material.bump_texname = mat.bump_texname;
        }
        if (!mat.emissive_texname.empty()) {
            material.emissive_texname = mat.emissive_texname;
        }

        mesh.materials[i] = material;
    }

    // Load verticies from the .obj file
    for (const auto& shape : shapes) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if(fv != 3) { // Should only be three due to triangulation
                ThrowError("number of verticies per face should always be three!");
            }
            int material_id = shape.mesh.material_ids[f];

            Face face;

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                face.vertex_indices[v] = mesh.vertices.size();

                Vertex vertex;
                vertex.material_id = material_id;

                vertex.vertex_cord = VertexCoord(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                );

                if (idx.normal_index >= 0) {
                    vertex.norm = Normal(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    );
                }

                if (idx.texcoord_index >= 0) {
                    vertex.tex_cord = TexCoord(
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    );
                }

                mesh.vertices.push_back(vertex);
            }
            mesh.faces.push_back(face);
            index_offset += fv;
        }
    }

    return mesh;
}

Mesh load_obj_file(const std::string& obj_filename, const std::string& mtl_base_dir) {
    _RawMesh raw_mesh;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&raw_mesh.attrib, &raw_mesh.shapes, &raw_mesh.materials, &err, obj_filename.c_str(), mtl_base_dir.c_str(), true);

    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "Error: " << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to load OBJ file!" << std::endl;
        return {};
    }
    return _load_obj_file(raw_mesh);
}

// ============================================================
//            Custom loading of obj files from memory
// ============================================================

class _CustomMaterialFileReader : public tinyobj::MaterialReader {
 public:
  explicit _CustomMaterialFileReader(std::map<std::string, std::vector<unsigned char>>* m_file_map_)
      : m_file_map(m_file_map_) {}
  virtual ~_CustomMaterialFileReader() {}
  virtual bool operator()(const std::string &matId,
                          std::vector<tinyobj::material_t> *materials,
                          std::map<std::string, int> *matMap, std::string *err);

 private:
  std::map<std::string, std::vector<unsigned char>>* m_file_map;
};

bool _CustomMaterialFileReader::operator()(const std::string &mat_filename,
                                    std::vector<tinyobj::material_t> *materials,
                                    std::map<std::string, int> *matMap,
                                    std::string *err) {
    // Iterate through all the files in the filemap
    // See if we can find the material file
    for(auto my_file: *m_file_map) {
        std::string raw_filename = vicmil::split_string(my_file.first, '/').back();
        if(raw_filename == mat_filename) {
            // Bingo! We found our file
            // Extract the content and return the result
            std::string file_contents = std::string((char*)(&my_file.second[0]), my_file.second.size());
            std::istringstream matIStream(file_contents);

            std::string warning;
            LoadMtl(matMap, materials, &matIStream, &warning);

            if (!warning.empty()) {
                if (err) {
                (*err) += warning;
                }
            }

            return true;
        }
    }

    // Could not locate a file with a matching name
    return false;
}

vicmil::Mesh load_obj_file_from_memory(std::map<std::string, std::vector<unsigned char>> file_map) {
    // file_map: map<file_path, file_content>
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    _CustomMaterialFileReader mtl_file_reader  = _CustomMaterialFileReader(&file_map);

    // Read the contents of an .obj file
    std::string file_contents = "";
    for(auto my_file: file_map) {
        std::string extension = vicmil::split_string(my_file.first, '.').back();
        if(extension == "obj") {
            file_contents = std::string((char*)(&my_file.second[0]), my_file.second.size());
            break;
        }
    }
    if (file_contents.size() == 0) {
        std::cerr << "Failed to parse .obj" << std::endl;
        return vicmil::Mesh();
    }
    
    // Load the model of the file into a format used by tinyobjloader
    vicmil::_RawMesh raw_mesh;
    std::istringstream inStream(file_contents);
    bool ret = tinyobj::LoadObj(&raw_mesh.attrib, &raw_mesh.shapes, &raw_mesh.materials, &err, &inStream, &mtl_file_reader, true);

    // Check for errors
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to parse .obj" << std::endl;
        return vicmil::Mesh();
    }

    // Convert the model into another format
    vicmil::Mesh mesh = vicmil::_load_obj_file(raw_mesh);
    return mesh;
}
}