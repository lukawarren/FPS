#pragma once
#include "common.h"
#include "window.h"
#include "mesh.h"
#include "camera.h"
#include "transform.h"
#include "texture.h"
#include "map.h"
#include "player.h"
#include "spotlight.h"

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
    SDL_GPUTexture* create_depth_texture();
    SDL_GPUTexture* create_shadow_map();

    SDL_GPUDevice* device;
    Window* window;
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
        glm::mat4 light_matrices[QUALITY_SETTINGS.max_shadows];
    } diffuse_shader_uniforms_fragment;

    SDL_GPUShader* depth_vertex_shader;
    SDL_GPUShader* depth_fragment_shader;
    SDL_GPUGraphicsPipeline* depth_pipeline;
    SDL_GPUTexture* shadow_map;
    SDL_GPUSampler* shadow_map_sampler;

    SDL_GPUSampler* sampler;

    Camera camera;
    Map* map;
    Player* player;
    Spotlight light;
};