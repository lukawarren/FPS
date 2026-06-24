#include "renderer.h"
#include "io.h"

Renderer::Renderer(const std::string& title, const u32 width, const u32 height)
{
    // Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        throw std::runtime_error(
            "Failed to initialise SDL: " + std::string(SDL_GetError())
        );

    if (!SDL_ShaderCross_Init())
        throw std::runtime_error("Failed to initialise SDL_ShaderCross: " + std::string(SDL_GetError()));

#ifdef __APPLE__
    if (is_debug())
        setenv("MTL_HUD_ENABLED", "1", 1);
#endif

    dbg("TODO: only need to change pipeline with swapchain format on resize?");

    device = SDL_CreateGPUDevice(get_shader_format(), is_debug(), get_backend());
    if (!device)
        throw std::runtime_error(
            "Failed to create GPU device: " + std::string(SDL_GetError())
        );

    // Fetch true width and height after window creation (e.g. for high-DPI)
    int w, h;
    window = new Window(title, width, height, device);
    SDL_GetWindowSizeInPixels(window->get_window(), &w, &h);
    this->framebuffer_width = w;
    this->framebuffer_height = h;
    framebuffer_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window->get_window());

    diffuse_texture = create_diffuse_texture();
    depth_texture = create_depth_texture();

    diffuse_vertex_shader = compile_shader("diffuse.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    diffuse_fragment_shader = compile_shader("diffuse.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    diffuse_pipeline = create_diffuse_pipeline();

    depth_vertex_shader = compile_shader("depth.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    depth_fragment_shader = compile_shader("depth.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    depth_pipeline = create_depth_pipeline();
    shadow_map = create_shadow_map();

    shadow_map_sampler = SDL_CreateGPUSampler(
        device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 0.0f,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .min_lod = 0.0f,
            .max_lod = FLT_MAX,
            .enable_anisotropy = false,
            .enable_compare = false,
            .props = 0
        }
    );

    sampler = SDL_CreateGPUSampler(
        device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 16.0f,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .min_lod = 0.0f,
            .max_lod = FLT_MAX,
            .enable_anisotropy = true,
            .enable_compare = false,
            .props = 0
        }
    );

    bloom_sampler = SDL_CreateGPUSampler(
        device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 0.0f,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .min_lod = 0.0f,
            .max_lod = FLT_MAX,
            .enable_anisotropy = false,
            .enable_compare = false,
            .props = 0
        }
    );

    quad_vertex_shader = compile_shader("quad.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    downsample_shader = compile_shader("downsample.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    composite_shader = compile_shader("composite.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
    bloom_textures = create_bloom_textures();
    downsample_pipeline = create_downsample_pipeline();
    composite_pipeline = create_composite_pipeline();

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    world = new World("map.map", window, device, copy_pass);
    quad = new Quad(device, copy_pass);
    SDL_EndGPUCopyPass(copy_pass);
    for (const auto& draw_call : world->map->draw_calls)
    {
        draw_call.texture->generate_mipmaps(command_buffer);
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

Renderer::~Renderer()
{
    SDL_WaitForGPUIdle(device);

    SDL_ReleaseGPUShader(device, diffuse_vertex_shader);
    SDL_ReleaseGPUShader(device, diffuse_fragment_shader);
    SDL_ReleaseGPUGraphicsPipeline(device, diffuse_pipeline);
    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseGPUTexture(device, diffuse_texture);

    SDL_ReleaseGPUGraphicsPipeline(device, depth_pipeline);
    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseGPUTexture(device, shadow_map);
    SDL_ReleaseGPUSampler(device, shadow_map_sampler);

    SDL_ReleaseGPUShader(device, quad_vertex_shader);
    SDL_ReleaseGPUShader(device, downsample_shader);
    SDL_ReleaseGPUShader(device, composite_shader);
    SDL_ReleaseGPUGraphicsPipeline(device, downsample_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(device, composite_pipeline);
    for (size_t i = 0; i < bloom_textures.size(); i++)
        SDL_ReleaseGPUTexture(device, bloom_textures[i]);
    SDL_ReleaseGPUSampler(device, bloom_sampler);


    delete world;
    delete quad;

    SDL_DestroyGPUDevice(device);
    SDL_ShaderCross_Quit();
}

bool Renderer::update()
{
    window->update();
    world->player.update(world->map->world, world->camera, 1.0f / 60.0f);
    world->player.update_camera(world->camera);
    return !window->should_close();
}

void Renderer::render()
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);

    // Get image from swapchain
    SDL_GPUTexture* swapchain_texture;
    u32 swapchain_width, swapchain_height;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window->get_window(), &swapchain_texture, &swapchain_width, &swapchain_height);
    if (swapchain_texture == nullptr)
        return;

    // Check if dimensions or format has changed
    SDL_GPUTextureFormat new_format = SDL_GetGPUSwapchainTextureFormat(device, window->get_window());
    if (swapchain_width != framebuffer_width ||
        swapchain_height != framebuffer_height ||
        new_format != framebuffer_texture_format)
    {
        SDL_ReleaseGPUTexture(device, diffuse_texture);
        SDL_ReleaseGPUTexture(device, depth_texture);
        for (size_t i = 0; i < bloom_textures.size(); i++)
            SDL_ReleaseGPUTexture(device, bloom_textures[i]);

        SDL_ReleaseGPUGraphicsPipeline(device, diffuse_pipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, downsample_pipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, composite_pipeline);

        framebuffer_width = swapchain_width;
        framebuffer_height = swapchain_height;
        framebuffer_texture_format = new_format;

        diffuse_pipeline = create_diffuse_pipeline();
        diffuse_texture = create_diffuse_texture();
        depth_texture = create_depth_texture();
        bloom_textures = create_bloom_textures();
        downsample_pipeline = create_downsample_pipeline();
        composite_pipeline = create_composite_pipeline();
    }

    const glm::mat4 camera_projection = world->camera.projection_matrix(framebuffer_width, framebuffer_height);
    const glm::mat4 camera_view = world->camera.view_matrix();

    const auto light_matrix = world->player.flashlight.get_matrix();

    // Set uniforms
    diffuse_shader_uniforms_vertex.view = camera_view;
    diffuse_shader_uniforms_vertex.projection = camera_projection;
    diffuse_shader_uniforms_fragment.spotlight = world->player.flashlight.get_uniform_buffer(light_matrix);

    {
        SDL_GPURenderPass* depth_pass = SDL_BeginGPURenderPass(
            command_buffer,
            NULL,
            0,
            &(SDL_GPUDepthStencilTargetInfo) {
                .texture = shadow_map,
                .clear_depth = 1.0f,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
                .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
                .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
                .cycle = false,
                .clear_stencil = 0,
                .mip_level = 0,
                .layer = 0
            }
        );

        SDL_BindGPUGraphicsPipeline(depth_pass, depth_pipeline);

        SDL_SetGPUViewport(depth_pass, &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = (float)QUALITY_SETTINGS.shadow_map_width,
            .h = (float)QUALITY_SETTINGS.shadow_map_height,
            .min_depth = 0.0f,
            .max_depth = 1.0f
        });

        SDL_PushGPUVertexUniformData(
            command_buffer,
            0,
            (void*)glm::value_ptr(light_matrix),
            sizeof(float) * 16
        );

        for (const auto& draw_call : world->map->draw_calls)
        {
            draw_call.mesh->bind(depth_pass);
            draw_call.mesh->draw(depth_pass);
        }

        SDL_EndGPURenderPass(depth_pass);
    }

    SDL_GPURenderPass* diffuse_pass = SDL_BeginGPURenderPass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
            .texture = diffuse_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = false,
            .cycle_resolve_texture = false
        },
        1,
        &(SDL_GPUDepthStencilTargetInfo) {
            .texture = depth_texture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = true,
            .clear_stencil = 0,
            .mip_level = 0,
            .layer = 0
        }
    );

    SDL_BindGPUGraphicsPipeline(diffuse_pass, diffuse_pipeline);

    SDL_SetGPUViewport(diffuse_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)swapchain_width,
        .h = (float)swapchain_height,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &diffuse_shader_uniforms_vertex,
        sizeof(diffuse_shader_uniforms_vertex)
    );

    SDL_PushGPUFragmentUniformData(
        command_buffer,
        0,
        &diffuse_shader_uniforms_fragment,
        sizeof(diffuse_shader_uniforms_fragment)
    );

    SDL_BindGPUFragmentSamplers(
        diffuse_pass,
        1,
        &(SDL_GPUTextureSamplerBinding) {
            .sampler = shadow_map_sampler,
            .texture = shadow_map
        },
        1
    );

    for (const auto& draw_call : world->map->draw_calls)
    {
        draw_call.texture->bind(diffuse_pass, sampler);
        draw_call.mesh->bind(diffuse_pass);
        draw_call.mesh->draw(diffuse_pass);
    }

    SDL_EndGPURenderPass(diffuse_pass);

    for (u32 level = 0; level < QUALITY_SETTINGS.bloom_downsamples; level++)
    {
        SDL_GPURenderPass* downsample_pass = SDL_BeginGPURenderPass(
            command_buffer,
            &(SDL_GPUColorTargetInfo) {
                .texture = bloom_textures[level],
                .mip_level = 0,
                .layer_or_depth_plane = 0,
                .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
                .load_op = SDL_GPU_LOADOP_DONT_CARE,
                .store_op = SDL_GPU_STOREOP_STORE,
                .resolve_texture = NULL,
                .resolve_mip_level = 0,
                .resolve_layer = 0,
                .cycle = false,
                .cycle_resolve_texture = false
            },
            1,
            NULL
        );

        SDL_BindGPUGraphicsPipeline(downsample_pass, downsample_pipeline);

        SDL_SetGPUViewport(downsample_pass, &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = (float)get_bloom_texture_width(level),
            .h = (float)get_bloom_texture_height(level),
            .min_depth = 0.0f,
            .max_depth = 1.0f
        });

        SDL_BindGPUFragmentSamplers(
            downsample_pass,
            0,
            &(SDL_GPUTextureSamplerBinding) {
                .sampler = bloom_sampler,
                .texture = (level == 0 ? diffuse_texture : bloom_textures[level - 1])
            },
            1
        );

        quad->bind(downsample_pass);
        quad->draw(downsample_pass);
        SDL_EndGPURenderPass(downsample_pass);
    }

    {
        SDL_GPURenderPass* composite_pass = SDL_BeginGPURenderPass(
            command_buffer,
            &(SDL_GPUColorTargetInfo) {
                .texture = swapchain_texture,
                .mip_level = 0,
                .layer_or_depth_plane = 0,
                .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
                .load_op = SDL_GPU_LOADOP_DONT_CARE,
                .store_op = SDL_GPU_STOREOP_STORE,
                .resolve_texture = NULL,
                .resolve_mip_level = 0,
                .resolve_layer = 0,
                .cycle = false,
                .cycle_resolve_texture = false
            },
            1,
            NULL
        );

        SDL_BindGPUGraphicsPipeline(composite_pass, composite_pipeline);

        SDL_SetGPUViewport(composite_pass, &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = (float)framebuffer_width,
            .h = (float)framebuffer_height,
            .min_depth = 0.0f,
            .max_depth = 1.0f
        });

        SDL_BindGPUFragmentSamplers(
            composite_pass,
            0,
            &(SDL_GPUTextureSamplerBinding) {
                .sampler = bloom_sampler,
                .texture = bloom_textures[bloom_textures.size() - 1]
            },
            1
        );

        quad->bind(composite_pass);
        quad->draw(composite_pass);
        SDL_EndGPURenderPass(composite_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

SDL_GPUShader* Renderer::compile_shader(
    const std::string& path,
    const SDL_ShaderCross_ShaderStage stage
)
{
    u8* source = io_read_file(SHADER_ROOT + path);

    SDL_ShaderCross_HLSL_Info hlsl_info = {};
    hlsl_info.source = (const char*)source;
    hlsl_info.entrypoint = "main";
    hlsl_info.include_dir = NULL;
    hlsl_info.defines = NULL;
    hlsl_info.shader_stage = stage;
    hlsl_info.props = 0;

    size_t bytecode_size;
    void* bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &bytecode_size);
    delete[] source;

    if (bytecode == NULL)
        throw std::runtime_error("Failed to compile shader " + std::string(path) + ": " + std::string(SDL_GetError()));

    SDL_ShaderCross_SPIRV_Info spirv_info = {};
    spirv_info.bytecode = (const u8*)bytecode;
    spirv_info.bytecode_size = bytecode_size;
    spirv_info.entrypoint = "main";
    spirv_info.shader_stage = stage;
    spirv_info.props = 0;

    SDL_ShaderCross_GraphicsShaderMetadata* reflect_info = SDL_ShaderCross_ReflectGraphicsSPIRV(
        (const u8*)bytecode,
        bytecode_size,
        0
    );

    if (reflect_info == NULL)
    {
        SDL_free(bytecode);
        throw std::runtime_error("Failed to reflect shader " + std::string(path) + ": " + std::string(SDL_GetError()));
    }

    SDL_GPUShader* shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        device,
        &spirv_info,
        &reflect_info->resource_info,
        0
    );

    if (shader == NULL)
    {
        SDL_free(bytecode);
        SDL_free(reflect_info);
        throw std::runtime_error("Failed to compile shader " + std::string(path) + " from SPIRV: " + std::string(SDL_GetError()));
    }

    SDL_free(bytecode);
    SDL_free(reflect_info);
    return shader;
}

SDL_GPUGraphicsPipeline* Renderer::create_diffuse_pipeline()
{
    const auto description = Mesh::Vertex::get_vertex_buffer_description();
    const auto attributes = Mesh::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = diffuse_vertex_shader,
        .fragment_shader = diffuse_fragment_shader,
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
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .has_depth_stencil_target = true
        },
        .props = 0
    });

    if (pipeline == NULL)
        throw std::runtime_error("Failed to create pipeline: " + std::string(SDL_GetError()));

    return pipeline;
}

SDL_GPUGraphicsPipeline* Renderer::create_depth_pipeline()
{
    const auto description = Mesh::Vertex::get_vertex_buffer_description();
    const auto attributes = Mesh::Vertex::get_vertex_attributes();
dbg("Todo: reflect format");
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = depth_vertex_shader,
        .fragment_shader = depth_fragment_shader,
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
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            .has_depth_stencil_target = true
        },
        .props = 0
    });

    if (pipeline == NULL)
        throw std::runtime_error("Failed to create pipeline: " + std::string(SDL_GetError()));

    return pipeline;
}

SDL_GPUGraphicsPipeline* Renderer::create_downsample_pipeline()
{
    const auto description = Quad::Vertex::get_vertex_buffer_description();
    const auto attributes = Quad::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = quad_vertex_shader,
        .fragment_shader = downsample_shader,
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

    if (pipeline == NULL)
        throw std::runtime_error("Failed to create pipeline: " + std::string(SDL_GetError()));

    return pipeline;
}

SDL_GPUGraphicsPipeline* Renderer::create_composite_pipeline()
{
    const auto description = Quad::Vertex::get_vertex_buffer_description();
    const auto attributes = Quad::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = framebuffer_texture_format;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = quad_vertex_shader,
        .fragment_shader = composite_shader,
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

    if (pipeline == NULL)
        throw std::runtime_error("Failed to create pipeline: " + std::string(SDL_GetError()));

    return pipeline;
}

std::array<SDL_GPUTexture*, QUALITY_SETTINGS.bloom_downsamples> Renderer::create_bloom_textures()
{
    std::array<SDL_GPUTexture*, QUALITY_SETTINGS.bloom_downsamples> textures;

    for (u32 i = 0; i < textures.size(); i++)
    {
        textures[i] = SDL_CreateGPUTexture(
            device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
                .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
                .width = get_bloom_texture_width(i),
                .height = get_bloom_texture_height(i),
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .props = 0
            }
        );
    }

    return textures;
}

u32 Renderer::get_bloom_texture_width(const u32 level)
{
    return std::max(framebuffer_width / (level + 2U), 16U);
}

u32 Renderer::get_bloom_texture_height(const u32 level)
{
    return std::max(framebuffer_height / (level + 2U), 16U);
}

SDL_GPUTexture* Renderer::create_diffuse_texture()
{
    return SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = framebuffer_width,
        .height = framebuffer_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

SDL_GPUTexture* Renderer::create_depth_texture()
{
    return SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = framebuffer_width,
        .height = framebuffer_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

SDL_GPUTexture* Renderer::create_shadow_map()
{
    const bool supports_32 = SDL_GPUTextureSupportsFormat(
        device,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTURETYPE_2D_ARRAY,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER
    );

    if (!supports_32)
        dbg("Warning: SDL_GPU_TEXTUREFORMAT_D32_FLOAT not supported");

    dbg("TODO: auto reflect in pipeline or whatever");

    return SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
        .format = supports_32 ? SDL_GPU_TEXTUREFORMAT_D32_FLOAT : SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = QUALITY_SETTINGS.shadow_map_width,
        .height = QUALITY_SETTINGS.shadow_map_height,
        .layer_count_or_depth = QUALITY_SETTINGS.max_shadows,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

bool Renderer::is_debug()
{
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}


SDL_GPUShaderFormat Renderer::get_shader_format()
{
#ifdef __APPLE__
    return SDL_GPU_SHADERFORMAT_MSL;
#else
    return SDL_GPU_SHADERFORMAT_SPIRV;
#endif
}

const char* Renderer::get_backend()
{
#ifdef __APPLE__
    return "metal";
#else
    return "vulkan";
#endif
}
