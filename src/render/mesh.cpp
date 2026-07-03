#include "render/mesh.h"

Mesh::Mesh(
    const std::vector<Vertex> vertices,
    const std::vector<u32> indices,
    SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass
)
{
    // Create GPU buffers
    const u32 vertex_size = (u32)sizeof(Vertex) * (u32)vertices.size();
    const u32 index_size = (u32)sizeof(u32) * (u32)indices.size();
    const SDL_GPUBufferCreateInfo vertex_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_size,
        .props = 0
    };
    const SDL_GPUBufferCreateInfo index_info = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = index_size,
        .props = 0
    };
    vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_info);
    index_buffer = SDL_CreateGPUBuffer(device, &index_info);

    // Create transfer buffer for uploading data
    const SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertex_size + index_size,
        .props = 0
    };
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &transfer_info
    );

    // Copy CPU-wise
    void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy((u8*)data, &vertices[0], vertex_size);
    memcpy((u8*)data + vertex_size, &indices[0], index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Copy GPU-wise
    SDL_GPUTransferBufferLocation location = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };
    SDL_GPUBufferRegion region = {
        .buffer = vertex_buffer,
        .offset = 0,
        .size = vertex_size
    };
    SDL_UploadToGPUBuffer(
        copy_pass,
        &location,
        &region,
        false
    );
    location.offset = vertex_size;
    region.buffer = index_buffer;
    region.size = index_size;
    SDL_UploadToGPUBuffer(
        copy_pass,
        &location,
        &region,
        false
    );

    // Destroy transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    n_indices = indices.size();
    this->device = device;
}

void Mesh::bind(SDL_GPURenderPass* render_pass) const
{
    const SDL_GPUBufferBinding vertex_binding = {
        .buffer = vertex_buffer,
        .offset = 0
    };

    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

    const SDL_GPUBufferBinding index_binding = {
        .buffer = index_buffer,
        .offset = 0
    };

    SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
}

void Mesh::draw(SDL_GPURenderPass* render_pass) const
{
    SDL_DrawGPUIndexedPrimitives(
        render_pass,
        n_indices,
        1,
        0,
        0,
        0
    );
}

Mesh::~Mesh()
{
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
}