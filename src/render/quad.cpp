#include "render/quad.h"

static const std::array<Quad::Vertex, 6> QUAD_VERTICES =
{{
    { glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 1.0f) }, // Bottom-Left
    { glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 1.0f) }, // Bottom-Right
    { glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 0.0f) }, // Top-Left
    { glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 0.0f) }, // Top-Left
    { glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 1.0f) }, // Bottom-Right
    { glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f, 0.0f) }  // Top-Right
}};

Quad::Quad(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    // Create GPU buffers
    const u32 vertex_size = (u32)sizeof(Vertex) * (u32)QUAD_VERTICES.size();
    const SDL_GPUBufferCreateInfo vertex_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_size,
        .props = 0
    };
    vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_info);

    // Create transfer buffer for uploading data
    const SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertex_size,
        .props = 0
    };
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &transfer_info
    );

    // Copy CPU-wise
    void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy((u8*)data, &QUAD_VERTICES[0], vertex_size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Copy GPU-wise
    const SDL_GPUTransferBufferLocation location = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };
    const SDL_GPUBufferRegion region = {
        .buffer = vertex_buffer,
        .offset = 0,
        .size = vertex_size
    };
    SDL_UploadToGPUBuffer(copy_pass, &location, &region, false);

    // Destroy transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    this->device = device;
}

void Quad::bind(SDL_GPURenderPass* render_pass) const
{
    const SDL_GPUBufferBinding binding = {
        .buffer = vertex_buffer,
        .offset = 0
    };
    SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);
}

void Quad::draw(SDL_GPURenderPass* render_pass) const
{
    SDL_DrawGPUPrimitives(
        render_pass,
        (u32)QUAD_VERTICES.size(),
        1,
        0,
        0
    );
}

Quad::~Quad()
{
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
}