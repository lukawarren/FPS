#include "mesh.h"

Mesh::Mesh(const std::vector<Vertex> vertices, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    // Create GPU buffer
    buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (u32)sizeof(Vertex) * (u32)vertices.size(),
        .props = 0
    });

    // Create transfer buffer for uploading data
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = (u32)sizeof(Vertex) * (u32)vertices.size(),
            .props = 0
        }
    );

    // Copy CPU-wise
    void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy(data, &vertices[0], (u32)sizeof(Vertex) * (u32)vertices.size());
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Copy GPU-wise
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = buffer,
            .offset = 0,
            .size = (u32)sizeof(Vertex) * (u32)vertices.size()
        },
        false
    );

    // Destroy transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    n_vertices = vertices.size();
}

void Mesh::bind(SDL_GPURenderPass* render_pass)
{
    SDL_BindGPUVertexBuffers(
        render_pass,
        0,
        &(SDL_GPUBufferBinding) {
            .buffer = buffer,
            .offset = 0
        },
        1
    );
}

void Mesh::draw(SDL_GPURenderPass* render_pass)
{
    SDL_DrawGPUPrimitives(
        render_pass,
        n_vertices,
        1,
        0,
        0
    );
}

Mesh::~Mesh()
{
    SDL_ReleaseGPUBuffer(device, buffer);
}