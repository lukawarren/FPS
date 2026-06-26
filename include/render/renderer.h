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
        Spotlight::UniformBuffer spotlights[QUALITY_SETTINGS.max_spotlights];
    } diffuse_shader_uniforms_fragment;

    struct alignas(16) SSAOShaderUniformsVertex
    {
        float aspect_ratio;
	    float tan_half_fov;
        float padding[2];
    } ssao_shader_uniforms_vertex;

    struct SSAOShaderUniformsFragment
    {
        glm::mat4 projection;
    } ssao_shader_uniforms_fragment;

    void shadow_pass(SDL_GPUCommandBuffer* command_buffer, const glm::mat4& light_matrix, const u8 slot);
    void depth_pass(SDL_GPUCommandBuffer* command_buffer, const glm::mat4& view, const glm::mat4& projection);
    void diffuse_pass(SDL_GPUCommandBuffer* command_buffer);
    void ssao_pass(SDL_GPUCommandBuffer* command_buffer);
    void ssao_blur_pass(SDL_GPUCommandBuffer* command_buffer);
    void downsample_pass(SDL_GPUCommandBuffer* command_buffer);
    void upsample_pass(SDL_GPUCommandBuffer* command_buffer);
    void composite_pass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture);

    Quad* quad;
    World* world;
};