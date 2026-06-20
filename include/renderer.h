#pragma once
#include "common.h"
#include "window.h"
#include "mesh.h"
#include "camera.h"
#include "transform.h"
#include "texture.h"
#include "map.h"

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
    u32 width, height;
    SDL_GPUTextureFormat framebuffer_texture_format;

    SDL_GPUShader* compile_shader(
        const std::string& path,
        const SDL_ShaderCross_ShaderStage stage
    );

    SDL_GPUGraphicsPipeline* create_graphics_pipeline(
        SDL_GPUShader* vertex_shader,
        SDL_GPUShader* fragment_shader
    );

    SDL_GPUTexture* create_depth_texture();

    SDL_GPUDevice* device;
    Window* window;
    SDL_GPUTexture* depth_texture;

    SDL_GPUShader* vertex_shader;
    SDL_GPUShader* fragment_shader;
    SDL_GPUGraphicsPipeline* pipeline;

    SDL_GPUSampler* sampler;

    Camera camera;
    Transform transform;
    Map* map;
};