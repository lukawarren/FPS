#include "render/texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(
    const std::string& filename,
    SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass,
    const std::string& root
)
{
    // Load image
    int channels, width, height;
    const std::string path = root + filename;
    uint8_t* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr)
        throw std::runtime_error("Failed to load texture " + path);

    // Need colour target for mipmap generation
    texture = SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = (u32)width,
        .height = (u32)height,
        .layer_count_or_depth = 1,
        .num_levels = (u32)std::floor(std::log2(std::max(width, height))) + 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });

    // Create transfer buffer for uploading data
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = (u32)sizeof(u32) * width * height,
            .props = 0
        }
    );

    // Copy CPU-wise
    void* data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy(data, pixels, (u32)sizeof(u32) * width * height);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Copy GPU-wise
    SDL_UploadToGPUTexture(
        copy_pass,
        &(SDL_GPUTextureTransferInfo) {
            .transfer_buffer = transfer_buffer,
            .offset = 0,
            .pixels_per_row = (u32)width,
            .rows_per_layer = (u32)height
        },
        &(SDL_GPUTextureRegion) {
            .texture = texture,
            .mip_level = 0,
            .layer = 0,
            .x = 0,
            .y = 0,
            .z = 0,
            .w = (u32)width,
            .h = (u32)height,
            .d = 1
        },
        false
    );

    // Destroy transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    stbi_image_free(pixels);
    this->device = device;
}

void Texture::bind(SDL_GPURenderPass* render_pass, SDL_GPUSampler* sampler)
{
    SDL_BindGPUFragmentSamplers(
        render_pass,
        0,
        &(SDL_GPUTextureSamplerBinding) {
            .sampler = sampler,
            .texture = texture
        },
        1
    );
}

void Texture::generate_mipmaps(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GenerateMipmapsForGPUTexture(command_buffer, texture);
}

Texture::~Texture()
{
    SDL_ReleaseGPUTexture(device, texture);
}
