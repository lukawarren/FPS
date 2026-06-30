#include "render/passes/sprite_pass.h"
#include "render/render_pass.h"

SpritePass::SpritePass(
    Device& device,
    PipelineFactory& pipeline_factory,
    TextureManager& texture_manager
) :
    device(device),
    pipeline_factory(pipeline_factory),
    texture_manager(texture_manager)
{
    dbg("TODO: instancing");
    dbg("TODO: sort by texture");
}

void SpritePass::execute(
    SDL_GPUCommandBuffer* command_buffer,
    const World& world,
    const std::unordered_map<Decal::ID, Texture*>& sprites,
    const glm::mat4& view,
    const glm::mat4& projection,
    const Quad& quad,
    const DiffusePass::FragmentUniforms& fragment_uniforms
)
{
    const RenderPass render_pass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
            .texture = texture_manager.diffuse_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = false,
            .cycle_resolve_texture = false
        },
        1,
        &(SDL_GPUDepthStencilTargetInfo) {
            .texture = texture_manager.depth_texture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = false,
            .clear_stencil = 0,
            .mip_level = 0,
            .layer = 0
        }
    );

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_factory.sprite_pipeline);

    const float width = (float)device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale;
    const float height = (float)device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale;
    render_pass.set_viewport(width, height);

    render_pass.push_view_projection(view, projection);

    SDL_PushGPUFragmentUniformData(
        command_buffer,
        0,
        &fragment_uniforms,
        sizeof(fragment_uniforms)
    );

    SDL_BindGPUFragmentSamplers(
        render_pass,
        1,
        &(SDL_GPUTextureSamplerBinding) {
            .sampler = texture_manager.shadow_map_sampler,
            .texture = texture_manager.shadow_map
        },
        1
    );

    VertexUniforms uniforms;
    quad.bind(render_pass);

    // TODO: sort by ID's
    sprites.at(Decal::ID::BULLET)->bind(render_pass, texture_manager.sampler);
    for (const auto& d : world.decals)
    {
        uniforms.model = d.transform.matrix();
        uniforms.spritesheet_info = d.get_spritesheet_info();
        SDL_PushGPUVertexUniformData(
            command_buffer,
            1,
            &uniforms,
            sizeof(uniforms)
        );

        quad.draw(render_pass);
    }

    // TODO: sort by ID's
    sprites.at(Decal::ID::ENEMY)->bind(render_pass, texture_manager.sampler);
    for (const auto& e : world.enemies)
    {
        uniforms.model = e->sprite.transform.matrix();
        uniforms.spritesheet_info = e->sprite.get_spritesheet_info();
        SDL_PushGPUVertexUniformData(
            command_buffer,
            1,
            &uniforms,
            sizeof(uniforms)
        );

        quad.draw(render_pass);
    }

    // TODO: sort by ID's
    sprites.at(Decal::ID::FIRE)->bind(render_pass, texture_manager.sampler);
    for (const auto& e : world.animated_sprites)
    {
        uniforms.model = e.transform.matrix();
        uniforms.spritesheet_info = e.get_spritesheet_info();
        SDL_PushGPUVertexUniformData(
            command_buffer,
            1,
            &uniforms,
            sizeof(uniforms)
        );

        quad.draw(render_pass);
    }
}
