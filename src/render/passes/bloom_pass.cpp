#include "render/passes/bloom_pass.h"
#include "render/render_pass.h"

BloomPass::BloomPass(
    Device& device,
    PipelineFactory& pipeline_factory,
    TextureManager& texture_manager,
    Quad& quad
) :
    device(device),
    pipeline_factory(pipeline_factory),
    texture_manager(texture_manager),
    quad(quad)
{}

void BloomPass::execute(SDL_GPUCommandBuffer* command_buffer)
{
    downsample(command_buffer);
    upsample(command_buffer);
}

void BloomPass::downsample(SDL_GPUCommandBuffer* command_buffer)
{
    for (u32 level = 0; level < QUALITY_SETTINGS.bloom_downsamples; level++)
    {
        const RenderPass<1> render_pass(
            command_buffer,
            {{{
                .texture = texture_manager.bloom_textures[level],
                .mip_level = 0,
                .layer_or_depth_plane = 0,
                .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
                .load_op = SDL_GPU_LOADOP_DONT_CARE,
                .store_op = SDL_GPU_STOREOP_STORE,
                .resolve_texture = NULL,
                .resolve_mip_level = 0,
                .resolve_layer = 0,
                .cycle = true,
                .cycle_resolve_texture = false
            }}}
        );

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline_factory.downsample_pipeline);

        render_pass.set_viewport(
            (float)texture_manager.get_bloom_texture_width(level),
            (float)texture_manager.get_bloom_texture_height(level)
        );

        render_pass.bind_fragment_sampler(
            0,
            (level == 0 ? texture_manager.diffuse_texture : texture_manager.bloom_textures[level - 1]),
            texture_manager.bloom_sampler
        );

        quad.bind(render_pass);
        quad.draw(render_pass);
    }
}

void BloomPass::upsample(SDL_GPUCommandBuffer* command_buffer)
{
    for (u32 level = 0; level < QUALITY_SETTINGS.bloom_downsamples - 1; level++)
    {
        const u32 target_level = (u32)texture_manager.bloom_textures.size() - level - 2;
        const u32 source_level = (u32)texture_manager.bloom_textures.size() - level - 1;

        const RenderPass<1> render_pass(
            command_buffer,
            {{{
                .texture = texture_manager.bloom_textures[target_level],
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
            }}}
        );

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline_factory.upsample_pipeline);

        render_pass.set_viewport(
            (float)texture_manager.get_bloom_texture_width(target_level),
            (float)texture_manager.get_bloom_texture_height(target_level)
        );

        render_pass.bind_fragment_sampler(
            0,
            texture_manager.bloom_textures[source_level],
            texture_manager.bloom_sampler
        );

        quad.bind(render_pass);
        quad.draw(render_pass);
    }
}
