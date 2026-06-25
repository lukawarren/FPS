#include "render/pipeline_factory.h"
#include "render/quad.h"
#include "render/mesh.h"

PipelineFactory::PipelineFactory(Device& device, const TextureManager& texture_manager) : device(device.device)
{
    depth_texture_array_format = texture_manager.depth_texture_array_format;

    diffuse_vs = device.compile_shader("diffuse.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    diffuse_fs = device.compile_shader("diffuse.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    depth_vs = device.compile_shader("depth.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    depth_fs = device.compile_shader("depth.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    quad_vs = device.compile_shader("quad.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    downsample_fs = device.compile_shader("downsample.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    upsample_fs = device.compile_shader("upsample.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    composite_fs = device.compile_shader("composite.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);

    diffuse_pipeline = create_diffuse_pipeline(
        diffuse_vs,
        diffuse_fs,
        SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        SDL_GPU_TEXTUREFORMAT_D16_UNORM
    );

    depth_pipeline = create_depth_pipeline(
        depth_vs,
        depth_fs,
        depth_texture_array_format
    );

    downsample_pipeline = create_downsample_pipeline(
        quad_vs,
        downsample_fs,
        SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT
    );

    upsample_pipeline = create_upsample_pipeline(
        quad_vs,
        upsample_fs,
        SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT
    );

    composite_pipeline = create_composite_pipeline(
        quad_vs,
        composite_fs,
        device.swapchain_format
    );
}

PipelineFactory::~PipelineFactory()
{
    SDL_ReleaseGPUShader(device, diffuse_vs);
    SDL_ReleaseGPUShader(device, diffuse_fs);
    SDL_ReleaseGPUShader(device, depth_vs);
    SDL_ReleaseGPUShader(device, depth_fs);
    SDL_ReleaseGPUShader(device, quad_vs);
    SDL_ReleaseGPUShader(device, downsample_fs);
    SDL_ReleaseGPUShader(device, upsample_fs);
    SDL_ReleaseGPUShader(device, composite_fs);

    SDL_ReleaseGPUGraphicsPipeline(device, diffuse_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, depth_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, downsample_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, upsample_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, composite_pipeline);
}

void PipelineFactory::on_swapchain_format_change(SDL_GPUTextureFormat swapchain_format)
{
    SDL_ReleaseGPUGraphicsPipeline(device, diffuse_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, downsample_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, upsample_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, composite_pipeline);

    diffuse_pipeline = create_diffuse_pipeline(
        diffuse_vs,
        diffuse_fs,
        SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        SDL_GPU_TEXTUREFORMAT_D16_UNORM
    );

    downsample_pipeline = create_downsample_pipeline(
        quad_vs,
        downsample_fs,
        SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT
    );

    upsample_pipeline = create_upsample_pipeline(
        quad_vs,
        downsample_fs,
        SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT
    );

    composite_pipeline = create_composite_pipeline(
        quad_vs,
        composite_fs,
        swapchain_format
    );
}

SDL_GPUGraphicsPipeline* PipelineFactory::create_diffuse_pipeline(
    SDL_GPUShader* vs,
    SDL_GPUShader* fs,
    SDL_GPUTextureFormat colour_format,
    SDL_GPUTextureFormat depth_format
)
{
    const auto description = Mesh::Vertex::get_vertex_buffer_description();
    const auto attributes = Mesh::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = colour_format;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = vs,
        .fragment_shader = fs,
        .vertex_input_state =
        {
            .vertex_buffer_descriptions = &description,
            .num_vertex_buffers = 1,
            .vertex_attributes = &attributes[0],
            .num_vertex_attributes = attributes.size()
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state =
        {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 0.0f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 0.0f,
            .enable_depth_bias = false,
            .enable_depth_clip = false
        },
        .multisample_state =
        {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .sample_mask = 0,
            .enable_mask = false,
            .enable_alpha_to_coverage = false
        },
        .depth_stencil_state =
        {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false
        },
        .target_info =
        {
            .color_target_descriptions = &colour_target,
            .num_color_targets = 1,
            .depth_stencil_format = depth_format,
            .has_depth_stencil_target = true
        },
        .props = 0
    });

    return check_pipeline(pipeline);
}

SDL_GPUGraphicsPipeline* PipelineFactory::create_depth_pipeline(
    SDL_GPUShader* vs,
    SDL_GPUShader* fs,
    SDL_GPUTextureFormat depth_format
)
{
    const auto description = Mesh::Vertex::get_vertex_buffer_description();
    const auto attributes = Mesh::Vertex::get_vertex_attributes();

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = vs,
        .fragment_shader = fs,
        .vertex_input_state =
        {
            .vertex_buffer_descriptions = &description,
            .num_vertex_buffers = 1,
            .vertex_attributes = &attributes[0],
            .num_vertex_attributes = attributes.size()
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state =
        {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 0.0f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 0.0f,
            .enable_depth_bias = false,
            .enable_depth_clip = false
        },
        .multisample_state =
        {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .sample_mask = 0,
            .enable_mask = false,
            .enable_alpha_to_coverage = false
        },
        .depth_stencil_state =
        {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false
        },
        .target_info =
        {
            .color_target_descriptions = NULL,
            .num_color_targets = 0,
            .depth_stencil_format = depth_format,
            .has_depth_stencil_target = true
        },
        .props = 0
    });

    return check_pipeline(pipeline);
}

SDL_GPUGraphicsPipeline* PipelineFactory::create_downsample_pipeline(
    SDL_GPUShader* vs,
    SDL_GPUShader* fs,
    SDL_GPUTextureFormat colour_format
)
{
    const auto description = Quad::Vertex::get_vertex_buffer_description();
    const auto attributes = Quad::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = colour_format;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = vs,
        .fragment_shader = fs,
        .vertex_input_state =
        {
            .vertex_buffer_descriptions = &description,
            .num_vertex_buffers = 1,
            .vertex_attributes = &attributes[0],
            .num_vertex_attributes = attributes.size()
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state =
        {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 0.0f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 0.0f,
            .enable_depth_bias = false,
            .enable_depth_clip = false
        },
        .multisample_state =
        {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .sample_mask = 0,
            .enable_mask = false,
            .enable_alpha_to_coverage = false
        },
        .depth_stencil_state =
        {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = false,
            .enable_depth_write = false,
            .enable_stencil_test = false
        },
        .target_info =
        {
            .color_target_descriptions = &colour_target,
            .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID,
            .has_depth_stencil_target = false
        },
        .props = 0
    });

    return check_pipeline(pipeline);
}

SDL_GPUGraphicsPipeline* PipelineFactory::create_upsample_pipeline(
    SDL_GPUShader* vs,
    SDL_GPUShader* fs,
    SDL_GPUTextureFormat colour_format
)
{
    const auto description = Quad::Vertex::get_vertex_buffer_description();
    const auto attributes = Quad::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = colour_format;
    colour_target.blend_state =
    {
        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .color_blend_op = SDL_GPU_BLENDOP_ADD,
        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
        .color_write_mask = 0,
        .enable_blend = true,
        .enable_color_write_mask = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = vs,
        .fragment_shader = fs,
        .vertex_input_state =
        {
            .vertex_buffer_descriptions = &description,
            .num_vertex_buffers = 1,
            .vertex_attributes = &attributes[0],
            .num_vertex_attributes = attributes.size()
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state =
        {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 0.0f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 0.0f,
            .enable_depth_bias = false,
            .enable_depth_clip = false
        },
        .multisample_state =
        {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .sample_mask = 0,
            .enable_mask = false,
            .enable_alpha_to_coverage = false
        },
        .depth_stencil_state =
        {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = false,
            .enable_depth_write = false,
            .enable_stencil_test = false
        },
        .target_info =
        {
            .color_target_descriptions = &colour_target,
            .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID,
            .has_depth_stencil_target = false
        },
        .props = 0
    });

    return check_pipeline(pipeline);
}

SDL_GPUGraphicsPipeline* PipelineFactory::create_composite_pipeline(
    SDL_GPUShader* vs,
    SDL_GPUShader* fs,
    SDL_GPUTextureFormat colour_format
)
{
    const auto description = Quad::Vertex::get_vertex_buffer_description();
    const auto attributes = Quad::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = colour_format;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = vs,
        .fragment_shader = fs,
        .vertex_input_state =
        {
            .vertex_buffer_descriptions = &description,
            .num_vertex_buffers = 1,
            .vertex_attributes = &attributes[0],
            .num_vertex_attributes = attributes.size()
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state =
        {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            .depth_bias_constant_factor = 0.0f,
            .depth_bias_clamp = 0.0f,
            .depth_bias_slope_factor = 0.0f,
            .enable_depth_bias = false,
            .enable_depth_clip = false
        },
        .multisample_state =
        {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .sample_mask = 0,
            .enable_mask = false,
            .enable_alpha_to_coverage = false
        },
        .depth_stencil_state =
        {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = false,
            .enable_depth_write = false,
            .enable_stencil_test = false
        },
        .target_info =
        {
            .color_target_descriptions = &colour_target,
            .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID,
            .has_depth_stencil_target = false
        },
        .props = 0
    });

    return check_pipeline(pipeline);
}

SDL_GPUGraphicsPipeline* PipelineFactory::check_pipeline(SDL_GPUGraphicsPipeline* pipeline) const
{
    if (pipeline == NULL)
        throw std::runtime_error("Failed to create pipeline: " + std::string(SDL_GetError()));
    return pipeline;
}