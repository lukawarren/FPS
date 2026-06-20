#pragma once
#include "common.h"

class Mesh
{
public:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 uv;

        static inline SDL_GPUVertexBufferDescription get_vertex_buffer_description()
        {
            return {
                .slot = 0,
                .pitch = sizeof(Vertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0,
            };
        }

        static inline std::array<SDL_GPUVertexAttribute, 2> get_vertex_attributes()
        {
            std::array<SDL_GPUVertexAttribute, 2> attributes;
            attributes[0].location = 0;
            attributes[0].buffer_slot = 0;
            attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            attributes[0].offset = 0;
            attributes[1].location = 1;
            attributes[1].buffer_slot = 0;
            attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            attributes[1].offset = offsetof(Vertex, uv);
            return attributes;
        }
    };

    Mesh(const std::vector<Vertex> vertices, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void bind(SDL_GPURenderPass* render_pass);
    void draw(SDL_GPURenderPass* render_pass);

private:
    SDL_GPUDevice* device;
    SDL_GPUBuffer* buffer;
    u32 n_vertices;
};