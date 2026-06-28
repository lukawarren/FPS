#pragma once
#include "common.h"
#include "world.h"
#include "render/quad.h"
#include "render/device.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"
#include "model.h"

class Renderer
{
public:
    Renderer(const std::string& title, const u32 width, const u32 height);
    ~Renderer();

    bool update();
    void render();

    std::unordered_map<Model::ID, Model*> models;

private:
    Device device;
    TextureManager texture_manager;
    PipelineFactory pipeline_factory;

    struct DiffuseShaderUniformsFragment
    {
        Spotlight::UniformBuffer spotlights[QUALITY_SETTINGS.max_spotlights];
    } diffuse_shader_uniforms_fragment;

    void shadow_pass(SDL_GPUCommandBuffer* command_buffer, const glm::mat4& light_matrix, const u8 slot);
    void depth_pass(
        SDL_GPUCommandBuffer* command_buffer,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::mat4& weapon_model,
        const glm::mat4& weapon_true_model,
        const glm::mat4& weapon_view,
        const glm::mat4& weapon_projection
    );
    void diffuse_pass(
        SDL_GPUCommandBuffer* command_buffer,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::mat4& weapon_model,
        const glm::mat4& weapon_true_model,
        const glm::mat4& weapon_view,
        const glm::mat4& weapon_projection
    );
    void downsample_pass(SDL_GPUCommandBuffer* command_buffer);
    void upsample_pass(SDL_GPUCommandBuffer* command_buffer);
    void composite_pass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture);

    Quad* quad;
    World* world;
};