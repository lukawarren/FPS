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

    device = SDL_CreateGPUDevice(get_shader_format(), is_debug(), get_backend());
    if (!device)
        throw std::runtime_error(
            "Failed to create GPU device: " + std::string(SDL_GetError())
        );

    window = new Window(title, width, height, device);
    this->width = width;
    this->height = height;
    framebuffer_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window->get_window());

    depth_texture = create_depth_texture();

    vertex_shader = compile_shader("main.vs.hlsl", SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
    fragment_shader = compile_shader("main.ps.hlsl", SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);

    pipeline = create_graphics_pipeline(
        vertex_shader,
        fragment_shader
    );

    sampler = SDL_CreateGPUSampler(
        device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
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

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    map = new Map("map.map", device, copy_pass);
    SDL_EndGPUCopyPass(copy_pass);
    for (const auto& draw_call : map->draw_calls)
    {
        draw_call.texture->generate_mipmaps(command_buffer);
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

    camera.position.z = 3;
    camera.position.y = 2;
    camera.pitch = 30.0f;
    player = new Player(window);
    player->setup_physics(map->world);
}

Renderer::~Renderer()
{
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseGPUSampler(device, sampler);

    delete player;
    delete map;

    SDL_DestroyGPUDevice(device);
    SDL_ShaderCross_Quit();
}

bool Renderer::update()
{
    window->update();
    player->update(map->world, camera, 1.0f / 60.0f);
    player->update_camera(camera);
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
    if (swapchain_width != width || swapchain_height != height || new_format != framebuffer_texture_format)
    {
        SDL_ReleaseGPUTexture(device, depth_texture);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);

        width = swapchain_width;
        height = swapchain_height;
        framebuffer_texture_format = new_format;

        pipeline = create_graphics_pipeline(vertex_shader, fragment_shader);
        depth_texture = create_depth_texture();
    }

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
            .texture = swapchain_texture,
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
            .cycle = true
        }
    );

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

    SDL_SetGPUViewport(render_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)swapchain_width,
        .h = (float)swapchain_height,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    const glm::mat4 m =
        camera.projection_matrix(swapchain_width, swapchain_height) *
        camera.view_matrix();

    SDL_PushGPUVertexUniformData(
        command_buffer,
        1,
        (void*)glm::value_ptr(m),
        sizeof(float) * 16
    );

    for (const auto& draw_call : map->draw_calls)
    {
        draw_call.texture->bind(render_pass, sampler);
        draw_call.mesh->bind(render_pass);
        draw_call.mesh->draw(render_pass);
    }

    SDL_EndGPURenderPass(render_pass);
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

SDL_GPUGraphicsPipeline* Renderer::create_graphics_pipeline(SDL_GPUShader* vertex_shader, SDL_GPUShader* fragment_shader)
{
    const auto description = Mesh::Vertex::get_vertex_buffer_description();
    const auto attributes = Mesh::Vertex::get_vertex_attributes();

    SDL_GPUColorTargetDescription colour_target;
    colour_target.format = framebuffer_texture_format;
    colour_target.blend_state =
    {
        .enable_blend = false
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo)
    {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
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

SDL_GPUTexture* Renderer::create_depth_texture()
{
    return SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
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
