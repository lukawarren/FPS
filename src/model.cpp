#include "model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Model::Model(const std::string& filename, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    texture = new Texture(filename + ".png", device, copy_pass, MODEL_ROOT);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err, warn;
    std::string model_path = MODEL_ROOT + filename + ".obj";

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), NULL))
        throw std::runtime_error(err);

    std::vector<Mesh::Vertex> vertices;
    std::vector<u32> indices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Mesh::Vertex vertex =
            {
                .position =
                {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                },
                .normal =
                {
                    attrib.normals[3 * index.vertex_index + 0],
                    attrib.normals[3 * index.vertex_index + 1],
                    attrib.normals[3 * index.vertex_index + 2]
                },
                .uv =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                }
            };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }

    mesh = new Mesh(vertices, indices, device, copy_pass);
}

Model::~Model()
{
    delete mesh;
    delete texture;
}