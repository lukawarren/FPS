#pragma once
#include "common.h"
#include "window.h"
#include "world.h"
#include "quad.h"

class Renderer
{
public:
    Renderer(const std::string& title, const u32 width, const u32 height);
    ~Renderer();

    bool update();
    void render();

private:
    SDL_GPUShaderFormat get_shader_format();
    const char* get_backend();
    bool is_debug();

    // Window resizing
    u32 framebuffer_width, framebuffer_height;
    SDL_GPUTextureFormat framebuffer_texture_format;

    SDL_GPUShader* compile_shader(
        const std::string& path,
        const SDL_ShaderCross_ShaderStage stage
    );

    SDL_GPUGraphicsPipeline* create_diffuse_pipeline();
    SDL_GPUGraphicsPipeline* create_depth_pipeline();
    SDL_GPUGraphicsPipeline* create_downsample_pipeline();
    SDL_GPUGraphicsPipeline* create_composite_pipeline();
    SDL_GPUTexture* create_diffuse_texture();
    SDL_GPUTexture* create_depth_texture();
    SDL_GPUTexture* create_shadow_map();
    std::array<SDL_GPUTexture*, QUALITY_SETTINGS.bloom_downsamples> create_bloom_textures();

    u32 get_bloom_texture_width(const u32 level);
    u32 get_bloom_texture_height(const u32 level);

    SDL_GPUDevice* device;
    Window* window;
    SDL_GPUTexture* diffuse_texture;
    SDL_GPUTexture* depth_texture;

    SDL_GPUShader* diffuse_vertex_shader;
    SDL_GPUShader* diffuse_fragment_shader;
    SDL_GPUGraphicsPipeline* diffuse_pipeline;

    struct alignas(16) PaddedFloat {
        float value;
    };

    struct DiffuseShaderUniformsVertex
    {
        glm::mat4 view;
        glm::mat4 projection;
    } diffuse_shader_uniforms_vertex;

    struct DiffuseShaderUniformsFragment
    {
        Spotlight::UniformBuffer spotlight;
    } diffuse_shader_uniforms_fragment;

    SDL_GPUShader* depth_vertex_shader;
    SDL_GPUShader* depth_fragment_shader;
    SDL_GPUGraphicsPipeline* depth_pipeline;
    SDL_GPUTexture* shadow_map;
    SDL_GPUSampler* shadow_map_sampler;

    SDL_GPUShader* quad_vertex_shader;
    SDL_GPUShader* downsample_shader;
    SDL_GPUShader* composite_shader;
    SDL_GPUGraphicsPipeline* downsample_pipeline;
    SDL_GPUGraphicsPipeline* composite_pipeline;
    std::array<SDL_GPUTexture*, QUALITY_SETTINGS.bloom_downsamples> bloom_textures;
    SDL_GPUSampler* bloom_sampler;
    Quad* quad;

    SDL_GPUSampler* sampler;

    World* world;
};