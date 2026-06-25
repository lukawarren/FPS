#pragma once
#include "common.h"
#include "world.h"
#include "render/quad.h"
#include "render/device.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"

class Renderer
{
public:
    Renderer(const std::string& title, const u32 width, const u32 height);
    ~Renderer();

    bool update();
    void render();

private:
    Device device;
    TextureManager texture_manager;
    PipelineFactory pipeline_factory;

    struct DiffuseShaderUniformsVertex
    {
        glm::mat4 view;
        glm::mat4 projection;
    } diffuse_shader_uniforms_vertex;

    struct DiffuseShaderUniformsFragment
    {
        Spotlight::UniformBuffer spotlight;
    } diffuse_shader_uniforms_fragment;

    void shadow_pass(SDL_GPUCommandBuffer* command_buffer, const glm::mat4& light_matrix);
    void diffuse_pass(SDL_GPUCommandBuffer* command_buffer);
    void downsample_pass(SDL_GPUCommandBuffer* command_buffer);
    void upsample_pass(SDL_GPUCommandBuffer* command_buffer);
    void composite_pass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture);

    Quad* quad;
    World* world;
};