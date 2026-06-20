#include "mesh.h"

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
    vertex_buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_size,
        .props = 0
    });
    index_buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = index_size,
        .props = 0
    });

    // Create transfer buffer for uploading data
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = vertex_size + index_size,
            .props = 0
        }
    );

    // Copy CPU-wise
    void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy((u8*)data, &vertices[0], vertex_size);
    memcpy((u8*)data + vertex_size, &indices[0], index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Copy GPU-wise
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = vertex_buffer,
            .offset = 0,
            .size = vertex_size
        },
        false
    );
    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = vertex_size
        },
        &(SDL_GPUBufferRegion) {
            .buffer = index_buffer,
            .offset = 0,
            .size = index_size
        },
        false
    );

    // Destroy transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    n_indices = indices.size();
    this->device = device;
}

void Mesh::bind(SDL_GPURenderPass* render_pass)
{
    SDL_BindGPUVertexBuffers(
        render_pass,
        0,
        &(SDL_GPUBufferBinding) {
            .buffer = vertex_buffer,
            .offset = 0
        },
        1
    );

    SDL_BindGPUIndexBuffer(
        render_pass,
        &(SDL_GPUBufferBinding) {
            .buffer = index_buffer,
            .offset = 0
        },
        SDL_GPU_INDEXELEMENTSIZE_32BIT
    );
}

void Mesh::draw(SDL_GPURenderPass* render_pass)
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